#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 200

#define CLR_U   "\033[1;35m"
#define CLR_H   "\033[1;34m"
#define CLR_P   "\033[1;36m"
#define CLR_OK  "\033[1;92m"
#define CLR_BAD "\033[1;91m"
#define CLR_RST "\033[0m"

typedef enum { IN_NONE, IN_FILE, IN_HEREDOC } InKind;
typedef enum { OUT_NONE, OUT_TRUNC, OUT_APPEND } OutKind;
typedef enum { ERR_NONE, ERR_TRUNC, ERR_APPEND } ErrKind;

typedef struct {
    char **argv;
    int argc;
    int cap;

    InKind in_kind;
    char *in_path;
    char *hd_delim;
    int in_fd;

    OutKind out_kind;
    char *out_path;
    int out_fd;

    ErrKind err_kind;
    char *err_path;
    int err_fd;
} Stage;

typedef struct {
    Stage *st;
    int n;
    int cap;
} Job;

typedef struct {
    char **tok;
    bool *quoted;
    int n;
    int cap;
} Lex;

static void fd_close(int *fd) {
    if (*fd != -1) { close(*fd); *fd = -1; }
}

static void must_dup(int a, int b) {
    if (dup2(a, b) == -1) { perror("dup2"); _exit(126); }
}

static void stage_init(Stage *c) {
    c->argv = NULL; c->argc = 0; c->cap = 0;
    c->in_kind = IN_NONE; c->in_path = NULL; c->hd_delim = NULL; c->in_fd = -1;
    c->out_kind = OUT_NONE; c->out_path = NULL; c->out_fd = -1;
    c->err_kind = ERR_NONE; c->err_path = NULL; c->err_fd = -1;
}

static void stage_free(Stage *c) {
    if (c->argv) {
        for (int i = 0; i < c->argc; i++) free(c->argv[i]);
        free(c->argv);
    }
    free(c->in_path);
    free(c->hd_delim);
    free(c->out_path);
    free(c->err_path);
    fd_close(&c->in_fd);
    fd_close(&c->out_fd);
    fd_close(&c->err_fd);
    stage_init(c);
}

static void arg_push(Stage *c, const char *s) {
    if (c->argc + 1 >= c->cap) {
        c->cap = (c->cap == 0) ? 8 : c->cap * 2;
        c->argv = (char**)realloc(c->argv, (size_t)c->cap * sizeof(char*));
        if (!c->argv) { perror("realloc"); exit(1); }
    }
    c->argv[c->argc++] = strdup(s);
    c->argv[c->argc] = NULL;
}

static void job_init(Job *j) {
    j->st = NULL; j->n = 0; j->cap = 0;
}

static void job_free(Job *j) {
    for (int i = 0; i < j->n; i++) stage_free(&j->st[i]);
    free(j->st);
    job_init(j);
}

static Stage* job_add(Job *j) {
    if (j->n >= j->cap) {
        j->cap = (j->cap == 0) ? 4 : j->cap * 2;
        j->st = (Stage*)realloc(j->st, (size_t)j->cap * sizeof(Stage));
        if (!j->st) { perror("realloc"); exit(1); }
    }
    stage_init(&j->st[j->n]);
    return &j->st[j->n++];
}

static const char* whoami_name(void) {
    const char *u = getenv("USER");
    if (u && *u) return u;
    struct passwd *pw = getpwuid(getuid());
    return (pw && pw->pw_name) ? pw->pw_name : "user";
}

static void hostname_get(char *buf, size_t n) {
    if (gethostname(buf, n) == -1) {
        strncpy(buf, "host", n);
        buf[n-1] = '\0';
    }
}

static const char* status_color(int code) {
    return (code == 0) ? CLR_OK : CLR_BAD;
}

static void prompt_show(int last) {
    char cwd[4096];
    char host[256];
    if (!getcwd(cwd, sizeof(cwd))) strcpy(cwd, "?");
    hostname_get(host, sizeof(host));
    printf(CLR_U "%s" CLR_RST "@" CLR_H "%s" CLR_RST ":" CLR_P "%s" CLR_RST " %s[%d]%s $ ",
           whoami_name(), host, cwd, status_color(last), last, CLR_RST);
    fflush(stdout);
}

static void lex_init(Lex *x) { x->tok = NULL; x->quoted = NULL; x->n = 0; x->cap = 0; }

static void lex_push(Lex *x, const char *s, bool q) {
    if (x->n >= x->cap) {
        x->cap = (x->cap == 0) ? 16 : x->cap * 2;
        x->tok = (char**)realloc(x->tok, (size_t)x->cap * sizeof(char*));
        x->quoted = (bool*)realloc(x->quoted, (size_t)x->cap * sizeof(bool));
        if (!x->tok || !x->quoted) { perror("realloc"); exit(1); }
    }
    x->tok[x->n] = strdup(s);
    x->quoted[x->n] = q;
    x->n++;
}

static void lex_free(Lex *x) {
    for (int i = 0; i < x->n; i++) free(x->tok[i]);
    free(x->tok);
    free(x->quoted);
    lex_init(x);
}

static bool is_op(char c) { return (c == '|' || c == '<' || c == '>'); }

static int lex_line(const char *line, Lex *out) {
    lex_init(out);
    size_t i = 0, L = strlen(line);

    while (i < L) {
        while (i < L && isspace((unsigned char)line[i])) i++;
        if (i >= L) break;

        if (line[i] == '2' && (i + 1 < L) && line[i+1] == '>') {
            if (i + 2 < L && line[i+2] == '>') { lex_push(out, "2>>", false); i += 3; }
            else { lex_push(out, "2>", false); i += 2; }
            continue;
        }

        if (is_op(line[i])) {
            char op[3] = {0,0,0};
            op[0] = line[i];
            if ((line[i] == '>' || line[i] == '<') && i + 1 < L && line[i+1] == line[i]) {
                op[1] = line[i+1];
                i += 2;
            } else i += 1;
            lex_push(out, op, false);
            continue;
        }

        if (line[i] == '"' || line[i] == '\'') {
            char q = line[i++];
            size_t s = i;
            while (i < L && line[i] != q) i++;
            size_t len = i - s;
            char *buf = (char*)malloc(len + 1);
            memcpy(buf, line + s, len);
            buf[len] = '\0';
            lex_push(out, buf, true);
            free(buf);
            if (i < L && line[i] == q) i++;
            continue;
        }

        size_t s = i;
        while (i < L && !isspace((unsigned char)line[i]) && !is_op(line[i])) {
            if (line[i] == '2' && (i + 1 < L) && line[i+1] == '>') break;
            i++;
        }
        size_t len = i - s;
        if (len == 0) continue;
        char *buf = (char*)malloc(len + 1);
        memcpy(buf, line + s, len);
        buf[len] = '\0';
        lex_push(out, buf, false);
        free(buf);
    }
    return out->n;
}

static bool has_glob(const char *s) {
    for (; *s; s++) if (*s == '*' || *s == '?' || *s == '[') return true;
    return false;
}

static void arg_push_glob(Stage *c, const char *s, bool quoted) {
    if (quoted || !has_glob(s)) { arg_push(c, s); return; }
    glob_t g;
    memset(&g, 0, sizeof(g));
    int r = glob(s, 0, NULL, &g);
    if (r == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) arg_push(c, g.gl_pathv[i]);
        globfree(&g);
        return;
    }
    globfree(&g);
    arg_push(c, s);
}

static int parse_job(const Lex *x, Job *j) {
    job_init(j);
    Stage *cur = job_add(j);

    for (int i = 0; i < x->n; i++) {
        const char *t = x->tok[i];

        if (strcmp(t, "|") == 0) {
            if (cur->argc == 0) { fprintf(stderr, "Error: pipe sin comando antes.\n"); job_free(j); return -1; }
            cur = job_add(j);
            continue;
        }

        if (strcmp(t, "<") == 0 || strcmp(t, "<<") == 0) {
            if (i + 1 >= x->n) { fprintf(stderr, "Error: falta argumento para %s\n", t); job_free(j); return -1; }
            const char *a = x->tok[++i];
            if (strcmp(t, "<") == 0) { cur->in_kind = IN_FILE; cur->in_path = strdup(a); }
            else { cur->in_kind = IN_HEREDOC; cur->hd_delim = strdup(a); }
            continue;
        }

        if (strcmp(t, ">") == 0 || strcmp(t, ">>") == 0) {
            if (i + 1 >= x->n) { fprintf(stderr, "Error: falta archivo para %s\n", t); job_free(j); return -1; }
            const char *a = x->tok[++i];
            cur->out_kind = (strcmp(t, ">>") == 0) ? OUT_APPEND : OUT_TRUNC;
            cur->out_path = strdup(a);
            continue;
        }

        if (strcmp(t, "2>") == 0 || strcmp(t, "2>>") == 0) {
            if (i + 1 >= x->n) { fprintf(stderr, "Error: falta archivo para %s\n", t); job_free(j); return -1; }
            const char *a = x->tok[++i];
            cur->err_kind = (strcmp(t, "2>>") == 0) ? ERR_APPEND : ERR_TRUNC;
            cur->err_path = strdup(a);
            continue;
        }

        arg_push_glob(cur, t, x->quoted[i]);
    }

    for (int i = 0; i < j->n; i++) {
        if (j->st[i].argc == 0) { fprintf(stderr, "Error: comando vacío.\n"); job_free(j); return -1; }
    }
    return 0;
}

static int heredoc_fd(const char *delim) {
    int fds[2];
    if (pipe(fds) == -1) { perror("pipe"); return -1; }
    char *line = NULL;
    size_t cap = 0;
    while (1) {
        fprintf(stdout, "hd> ");
        fflush(stdout);
        ssize_t n = getline(&line, &cap, stdin);
        if (n < 0) break;
        if (n > 0 && line[n-1] == '\n') line[n-1] = '\0';
        if (strcmp(line, delim) == 0) break;
        dprintf(fds[1], "%s\n", line);
    }
    free(line);
    close(fds[1]);
    return fds[0];
}

static int prep_redirs(Job *j) {
    for (int i = 0; i < j->n; i++) {
        Stage *c = &j->st[i];

        if (c->in_kind == IN_FILE) {
            c->in_fd = open(c->in_path, O_RDONLY);
            if (c->in_fd == -1) { fprintf(stderr, "Error abriendo %s: %s\n", c->in_path, strerror(errno)); return -1; }
        } else if (c->in_kind == IN_HEREDOC) {
            c->in_fd = heredoc_fd(c->hd_delim);
            if (c->in_fd == -1) return -1;
        }

        if (c->out_kind != OUT_NONE) {
            int flags = O_CREAT | O_WRONLY | ((c->out_kind == OUT_APPEND) ? O_APPEND : O_TRUNC);
            c->out_fd = open(c->out_path, flags, 0644);
            if (c->out_fd == -1) { fprintf(stderr, "Error abriendo %s: %s\n", c->out_path, strerror(errno)); return -1; }
        }

        if (c->err_kind != ERR_NONE) {
            int flags = O_CREAT | O_WRONLY | ((c->err_kind == ERR_APPEND) ? O_APPEND : O_TRUNC);
            c->err_fd = open(c->err_path, flags, 0644);
            if (c->err_fd == -1) { fprintf(stderr, "Error abriendo %s: %s\n", c->err_path, strerror(errno)); return -1; }
        }
    }
    return 0;
}

static void child_signals_default(void) {
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}

static int status_code(int st) {
    if (WIFEXITED(st)) return WEXITSTATUS(st);
    if (WIFSIGNALED(st)) return 128 + WTERMSIG(st);
    return 1;
}

static int run_job(Job *j) {
    if (prep_redirs(j) != 0) return 1;
    if (j->n <= 0) return 1;
    if (j->n > 1024) { fprintf(stderr, "Error: pipeline demasiado largo\n"); return 1; }

    size_t n = (size_t)j->n;
    pid_t *pids = calloc(n, sizeof *pids);
    if (!pids) { perror("calloc"); return 1; }

    int prev_in = -1;

    for (int i = 0; i < j->n; i++) {
        int pf[2] = {-1, -1};
        bool has_next = (i < j->n - 1);
        if (has_next) {
            if (pipe(pf) == -1) { perror("pipe"); free(pids); return 1; }
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            fd_close(&pf[0]); fd_close(&pf[1]);
            free(pids);
            return 1;
        }

        if (pid == 0) {
            child_signals_default();

            if (j->st[i].in_fd != -1) must_dup(j->st[i].in_fd, STDIN_FILENO);
            else if (prev_in != -1) must_dup(prev_in, STDIN_FILENO);

            if (j->st[i].out_fd != -1) must_dup(j->st[i].out_fd, STDOUT_FILENO);
            else if (has_next) must_dup(pf[1], STDOUT_FILENO);

            if (j->st[i].err_fd != -1) must_dup(j->st[i].err_fd, STDERR_FILENO);

            fd_close(&pf[0]); fd_close(&pf[1]);
            fd_close(&prev_in);

            for (int k = 0; k < j->n; k++) {
                fd_close(&j->st[k].in_fd);
                fd_close(&j->st[k].out_fd);
                fd_close(&j->st[k].err_fd);
            }

            execvp(j->st[i].argv[0], j->st[i].argv);
            fprintf(stderr, "Error execvp(%s): %s\n", j->st[i].argv[0], strerror(errno));
            _exit(127);
        }

        pids[i] = pid;

        fd_close(&prev_in);
        if (has_next) {
            fd_close(&pf[1]);
            prev_in = pf[0];
            pf[0] = -1;
        } else {
            fd_close(&pf[0]); fd_close(&pf[1]);
        }
    }

    int last = 0;
    for (int i = 0; i < j->n; i++) {
        int st = 0;
        waitpid(pids[i], &st, 0);
        if (i == j->n - 1) last = st;
    }

    free(pids);
    return status_code(last);
}

static bool is_cd(const Job *j) {
    return (j->n == 1 && j->st[0].argc > 0 && strcmp(j->st[0].argv[0], "cd") == 0);
}

static int run_cd(const Job *j) {
    const char *target = NULL;
    if (j->st[0].argc >= 2) target = j->st[0].argv[1];
    else target = getenv("HOME");
    if (!target) target = "/";
    if (chdir(target) == -1) { fprintf(stderr, "cd: %s: %s\n", target, strerror(errno)); return 1; }
    return 0;
}

static bool is_exit_cmd(const Job *j) {
    return (j->n == 1 && j->st[0].argc > 0 && strcmp(j->st[0].argv[0], "exit") == 0);
}

int main(void) {
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    char buf[4096];
    int last = 0;

    while (1) {
        prompt_show(last);

        if (!fgets(buf, sizeof(buf), stdin)) { printf("\n"); break; }
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';

        if (strlen(buf) > MAX_LINE) { fprintf(stderr, "Error: máximo %d caracteres.\n", MAX_LINE); last = 1; continue; }

        char *s = buf;
        while (*s && isspace((unsigned char)*s)) s++;
        if (*s == '\0') continue;

        Lex lx;
        if (lex_line(buf, &lx) <= 0) { lex_free(&lx); continue; }

        Job job;
        if (parse_job(&lx, &job) != 0) { lex_free(&lx); last = 1; continue; }
        lex_free(&lx);

        if (is_exit_cmd(&job)) { job_free(&job); break; }
        if (is_cd(&job)) { last = run_cd(&job); job_free(&job); continue; }

        last = run_job(&job);
        job_free(&job);
    }

    return 0;
}