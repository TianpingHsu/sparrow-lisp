#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

typedef struct object* (*primitive_t)(struct object* args);
typedef struct object* (*syntax_t)(struct object*, struct object* );

struct object {
    enum {
        BOOLEAN, NUMBER, SYMBOL, STRING, PORT, LIST, PROCEDURE, PRIMITIVE, ENVIRONMENT, SYNTAX
    } type;
    union {
        bool b;
        int64_t interger;
        char* s;  // STRING or SYMBOL
        FILE* stream;
        struct {  // LIST
            struct object *car;
            struct object *cdr;
        };
        struct {  // PROCEDURE
            struct object *params;
            struct object *body;
            struct object *env;
        };
        primitive_t primitive;
        syntax_t syntax;  // spefical forms
        struct {  // ENVIRONMENT
            struct object* frame;  // list of pairs: ((vars), (vals))
            struct object* parent;  // parent env
        };
    };
};

static struct {
    struct object** table;
    int size;
} g_sym_table;
static struct object* the_global_environment = NULL;
#define newline() printf("\n")
#define PRINT(exp) do { \
    newline(); \
    print(exp); \
    newline(); \
} while(0)
#define REQUIRE(exp, TYPE) do { \
    static const char* types[] = \
    {"boolean", "number", "symbol", "string", "port", "list", "procedure", "environmen", "syntax"};\
    if ((!exp && TYPE != LIST) || (exp && exp->type != TYPE)) { \
        printf("require type: %s, but exp has type: %s\n", types[TYPE], \
                exp ? types[LIST] : types[exp->type]);\
        assert(0); \
    } \
} while(0)
void tracepoint() {printf("tracepoint\n");}

/*========================================================
 * basics
 * =======================================================*/
struct object* mk_obj(int t)
{
    //printf("size: %lu\n", sizeof(struct object));
    struct object* o = malloc(sizeof(struct object));
    memset(o, 0, sizeof(struct object));
    o->type = t;
    return o;
}

struct object* mk_bool(bool b) {
    struct object* o = mk_obj(BOOLEAN);
    o->b = b;
    return o;
}

struct object* mk_integer(int64_t x) {
    struct object* o = mk_obj(NUMBER);
    o->interger = x;
    return o;
}

extern char* strdup(const char*);
struct object* mk_str(const char* s) {
    struct object* o = NULL;
    if (s) {
        o = mk_obj(STRING);
        o->s = strdup(s);
    }
    return o;
}

static unsigned long hash(const char* str) {
    // check [here](http://www.cse.yorku.ca/~oz/hash.html)
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash % g_sym_table.size;  // collision is possible
}

struct object* mk_sym(const char* s) {
    unsigned long k = hash(s);
    //printf("%s: %lu\n", s, k);
    struct object* o = g_sym_table.table[k];
    if (!o) {
        o = mk_str(s);
        o->type = SYMBOL;
        g_sym_table.table[k] = o;
    }
    return o;
}

struct object* cons(struct object*  x, struct object* y) {
    struct object* o = mk_obj(LIST);
    o->car = x;
    o->cdr = y;
    return o;
}

struct object* list(const unsigned int num, ...) {
    if (!num) return NULL;
    struct object **l = malloc(sizeof(struct object*) * num);
    va_list valist;
    va_start(valist, num);
    for (int i = 0; i < num; i++) {
        l[i] = va_arg(valist, struct object*);
    }
    va_end(valist);
    struct object* ret = NULL;
    for (int i = num - 1; i >= 0; i--) {
        ret = cons(l[i], ret);
    }
    free(l);
    return ret;
}

struct object* mk_procedure(struct object* params, struct object* body, struct object* env) {
    struct object* o = mk_obj(PROCEDURE);
    o->params = params;
    o->body = body;
    o->env = env;
    return o;
}

struct object* mk_prim(primitive_t prim) {
    struct object* o = mk_obj(PRIMITIVE);
    o->primitive = prim;
    return o;
}

struct object* mk_env(struct object* parent) {
    struct object* new_env = mk_obj(ENVIRONMENT);
    new_env->frame = cons(NULL, NULL);
    new_env->parent = parent;
    return new_env;
}

struct object* mk_syntax(syntax_t p)
{
    struct object* o =mk_obj(SYNTAX);
    o->syntax = p;
    return o;
}

struct object* car(struct object* l) {return l->car;}
struct object* cdr(struct object* l) {return l->cdr;}
struct object* caar(struct object* l) {return l->car->car;}
struct object* cadr(struct object* l) {return l->cdr->car;}
struct object* cdar(struct object* l) {return l->car->cdr;}
struct object* cddr(struct object* l) {return l->cdr->cdr;}
struct object* caddr(struct object* l) {return l->cdr->cdr->car;}

void print(struct object* o) {
    if (!o) {
        printf("()");
    } else {
        switch (o->type) {
            case BOOLEAN:
                printf("%s", o->b ? "#t" : "#f");
                break;
            case NUMBER:
                printf("%ld", o->interger);
                break;
            case SYMBOL:
                printf("%s", o->s);
                break;
            case STRING:
                printf("\"%s\"", o->s);
                break;
            case PORT:
                printf("<PORT>");
                break;
            case LIST:
                {
                    printf("(");
                    while (o) {
                        print(car(o));
                        if (cdr(o)) {
                            printf(" ");
                            if (cdr(o)->type != LIST) {
                                printf(". ");
                                print(cdr(o));
                                break;
                            } else {
                                o = cdr(o);
                            }
                        } else {
                            break;
                        }
                    }
                    printf(")");
                }
                break;
            case PRIMITIVE:
                printf("<BUILTIN-PRIMITIVE:%p>", o->primitive);
                break;
            case PROCEDURE:
                printf("<COMPOUND-PROCEDURE> #parent:%p", o->parent);
                break;
            case ENVIRONMENT:
                while (o) {
                    struct object* frame = o->frame;
                    struct object* vars = car(frame);
                    struct object* vals = cdr(frame);
                    while (vars) {
                        print(car(vars));
                        printf(" : ");
                        print(car(vals));
                        newline();
                        vars = cdr(vars);
                        vals = cdr(vals);
                    }
                    o = o->parent;
                }
                break;
            case SYNTAX:
                printf("SPECIAL-FORMS");
                break;
            default:
                printf("DEFAULT");
                break;
        }
    }
}

/*========================================================
 * builtins
 * =======================================================*/

struct object* lookup_variable(struct object* var, struct object* env) {
    while (env) {
        struct object* frame = env->frame;
        struct object* vars = car(frame);
        struct object* vals = cdr(frame);
        while (vars) {
            if (var == car(vars)) return car(vals);
            vars = cdr(vars);
            vals = cdr(vals);;
        }
        env = env->parent;
    }
    return NULL; 
}

struct object* set_variable(struct object* var, struct object* val, struct object* env) {
    while (env) {
        struct object* frame = env->frame;
        struct object* vars = car(frame);
        struct object* vals = cdr(frame);
        while (vars) {
            if (var == car(vars)) {
                vals->car = val;
                break;
            }
            vars = cdr(vars);
            vals = cdr(vals);
        }
        env = cdr(env);
    }
    return val;
}

struct object* define_variable(struct object* var, struct object* val, struct object* env) {
    struct object* frame = env->frame;
    struct object* vars = car(frame);
    struct object* vals = cdr(frame);
    while (vars) {
        if (var == car(vars)) {
            vals->car = val;
            return val;
        }
        vars = cdr(vars);
        vals = cdr(vals);
    }
    frame->car = cons(var, car(frame));
    frame->cdr = cons(val, cdr(frame));
    return val;
}

struct object* extend_environment(struct object* vars, struct object* vals, struct object* env) {
    struct object* new_env = mk_env(env);
    new_env->frame = cons(vars, vals);
    return new_env;
}

struct object* prim_cons(struct object* l) {
    // (cons x y)
    return cons(cadr(l), caddr(l));
}

struct object* prim_car(struct object* l) {
    // (car l)
    REQUIRE(cadr(l), LIST);
    return car(cadr(l));
}

struct object* prim_cdr(struct object* l) {
    // (cdr l)
    REQUIRE(cadr(l), LIST);
    return cdr(cadr(l));
}

struct object* prim_eq(struct object* exp) {
    // (equal x y)
    exp = cdr(exp);
    struct object* x = car(exp);
    struct object* y = cdr(exp);
    bool b = false;
    if (x == NULL || y == NULL) {
        b = (x == y);
    } else if (x->type == y->type) {
        switch (x->type) {
            case NUMBER:
                b = (x->interger == y->interger);
                break;
            case SYMBOL:
                b = (!strcmp(x->s, y->s));
                break;
            default:
                b = (x == y);
                break;
        }
    }
    return mk_bool(b);
}

struct object* prim_isnull(struct object* exp) {
    // (null? l)
    struct object* empty_list = mk_sym("()");
    struct object* l = cadr(exp);
    return prim_eq(cons(empty_list, l));
}

struct object* prim_add(struct object* l) {
    // (+ x ...)
    l = cdr(l);
    int64_t sum = car(l)->interger;
    while ((l = cdr(l))) {
        sum += car(l)->interger;
    }
    return mk_integer(sum);
}

struct object* prim_multiply(struct object* l) {
    // (* x ...)
    l = cdr(l);
    int64_t product = car(l)->interger;
    while ((l = cdr(l))) {
        product *= car(l)->interger;
    }
    return mk_integer(product);
}

struct object* prim_subtract(struct object* l) {
    // (- x ...)
    l = cdr(l);
    int64_t sum = car(l)->interger;
    while ((l = cdr(l))) {
        sum -= car(l)->interger;
    }
    return mk_integer(sum);
}

struct object* prim_divide(struct object* l) {
    // (- x y)
    l = cdr(l);
    int64_t quot = car(l)->interger;
    quot /= cadr(l)->interger;
    return mk_integer(quot);
}

struct object* prim_mod(struct object* l) {
    // (mod x y)
    l = cdr(l);
    int64_t quot = car(l)->interger;
    quot %= cadr(l)->interger;
    return mk_integer(quot);
}


struct object* eval(struct object* exp, struct object* env);
struct object* syntax_if(struct object* exp, struct object* env) {
    // (if predicate consequent alternative)
    struct object* predicate = cadr(exp);
    if (eval(predicate, env)) {
        struct object* consequent = caddr(exp);
        return eval(consequent, env);
    } else {
        struct object* alternative = cadr(cddr(exp));
        return eval(alternative, env);
    }
}

struct object* syntax_quote(struct object* exp, struct object* env) {
    // (quote <datum>)
    //return eval(cdr(exp), env);
    return cadr(exp);
}

struct object* syntax_define(struct object* exp, struct object* env) {
    if (cadr(exp)->type == LIST) {
        // (define (<var> <param1> <param2> ...) (<body>))
        struct object* var = car(cadr(exp));  // (var ...)
        struct object* params = cdr(cadr(exp));
        struct object* body = caddr(exp);
        return define_variable(var, mk_procedure(params, body, env), env);
    } else {
        // (define <var> <val>)
        return define_variable(cadr(exp), eval(caddr(exp), env), env);
    }
}

struct object* syntax_lambda(struct object* exp, struct object* env) {
    // (lambda (<params>) <body>)
    struct object* params = cadr(exp);
    struct object* body = caddr(exp);
    struct object* closure = mk_procedure(params, body, env);
    return closure;
}

struct object* eval_list(struct object* exp, struct object* env) {
    struct object* func = eval(car(exp), env);  // eval operator
    switch (func->type) {
        case SYNTAX:  // special forms
                return (func->syntax)(exp, env);
        case PRIMITIVE:
                {
                    struct object* args = cdr(exp), *l = NULL;
                    while (args) {
                        l = cons(eval(car(args), env), l);
                        args = cdr(args);
                    }
                    exp = cons(func, l);
                    return (func->primitive)(exp);
                }
        default:
            {
                // (func <args>)
                // eval operands
                struct object* params = func->params;
                struct object* args = cdr(exp);
                struct object* new_env = mk_env(func->env);
                while (params) {
                    define_variable(car(params), eval(car(args), env), new_env);
                    params = cdr(params);
                    args = cdr(args);
                }
                //PRINT(new_env);

                // apply
                struct object* body = func->body;
                //print(body);
                newline();
                return eval(body, new_env);
            }
            break;
    }
    return NULL;
}

struct object* eval(struct object* exp, struct object* env) {
    if (!exp) return mk_sym("()");
    switch (exp->type) {
        case NUMBER:
        case STRING:
            return exp;
        case SYMBOL:
            {
                struct object* val = lookup_variable(exp, env);
                if (!val) {
                    printf("Unbound Symbol: %s\n", exp->s) ;
                    return mk_sym("*Unbound*");
                }
                return val;
            }
        case LIST:
            return eval_list(exp, env);
           break;
        default:
            break;
    }
    return NULL;
}

/*========================================================
 * parser
 * =======================================================*/
int peek(FILE* fp) {
    int c = getc(fp);
    ungetc(c, fp);
    return c;
}

bool is_white_space(int c) { return (c == ' ' || c == '\n' || c == '\r' || c == '\t'); }
char SYMBOLS[] = "~!@#$%^&*_-+\\:,.<>|{}[]?=/";
struct object *read_symbol(FILE *in, char start) {
    char buf[128];
    buf[0] = start;
    int i = 1;
    while (isalnum(peek(in)) || strchr(SYMBOLS, peek(in))) {
        if (i >= 128)
            printf("Symbol name too long - maximum length 128 characters");
        buf[i++] = getc(in);
    }
    buf[i] = '\0';
    return mk_sym(buf);
}

struct object* reverse(struct object* l, struct object* base) {
    if (!l) return base;
    return reverse(cdr(l), cons(car(l), base));
}

struct object* read_exp(FILE* fp);
struct object* read_list(FILE *fp) {
    struct object* l = NULL;
    while (true) {
        struct object* o = read_exp(fp);
        if (o == NULL) break;
        l = cons(o, l);
    }
    return reverse(l, NULL);
}


struct object* read_exp(FILE* fp) {
    int c = peek(fp);
    for (;;) {
        c = getc(fp);
        if (is_white_space(c)) continue;

        if (c == ';') {  // skip comment
            while (true) {
                c = getc(fp);
                if (c == '\n' || c == EOF) break;
            }
            continue;
        }

        if (c == '"') {  // read string
            static char buf[256] = {0};
            int len = 0;
            while ((c = getc(fp) != EOF)) {
               c = getc(fp); 
               if (c == '"') {
                   buf[len] = '\0';
                   return mk_str(buf);
               }
               if (len == 255) {
                   printf("string is too long\n");
                   buf[len++] = '\0';
                   return mk_str(buf);
               }
               buf[len++] = c;
            }
            return NULL;
        }

        if (c == '\'') {
            struct object* quoted_exp = read_exp(fp);
            return cons(mk_sym("quote"), cons(quoted_exp, NULL));
        }
        
        if (isdigit(c) || ((c == '-') && isdigit(peek(fp)))) {
            int sign = (c == '-') ? -1 : 1;
            int sum = (sign == -1) ? 0 : (c - '0');
            while (isdigit(peek(fp))) {
                sum = sum * 10 + (getc(fp) - '0');
            }
            return mk_integer(sign * sum);
        }

        if (c == '(') return read_list(fp);
        if (c == ')') return NULL;

        if (isalpha(c) || strchr(SYMBOLS, c)) return read_symbol(fp, c);

    }
    return NULL;
}

struct object* load(const char* filename) {
    FILE* fp = fopen(filename, "r");
    struct object* exp = NULL;
    while (peek(fp) != EOF) {
        exp = read_exp(fp);
        print(exp);
        newline();
    }
    fclose(fp);
    return exp;
}

/*========================================================
 * init
 * =======================================================*/
static void sparrow_init() {
    // init symbol table
    {
        g_sym_table.size = 8191;
        g_sym_table.table = malloc(sizeof(struct object*) * g_sym_table.size);
        memset(g_sym_table.table, 0, sizeof(struct object*) * g_sym_table.size);
    }

    // init the_global_environment
    {
        the_global_environment = mk_env(NULL);
        struct object* e_true = mk_bool(true);
        struct object* e_false = mk_bool(false);
        define_variable(mk_sym("#t"), e_true, the_global_environment);
        define_variable(mk_sym("true"), e_true, the_global_environment);
        define_variable(mk_sym("#f"), e_false, the_global_environment);
        define_variable(mk_sym("false"), e_false, the_global_environment);
        define_variable(mk_sym("()"), NULL, the_global_environment);

        // primitives
        define_variable(mk_sym("cons"), mk_prim(prim_cons), the_global_environment);
        define_variable(mk_sym("car"), mk_prim(prim_car), the_global_environment);
        define_variable(mk_sym("cdr"), mk_prim(prim_cdr), the_global_environment);
        define_variable(mk_sym("equal?"), mk_prim(prim_eq), the_global_environment);
        define_variable(mk_sym("null?"), mk_prim(prim_isnull), the_global_environment);
        define_variable(mk_sym("+"), mk_prim(prim_add), the_global_environment);
        define_variable(mk_sym("*"), mk_prim(prim_multiply), the_global_environment);
        define_variable(mk_sym("-"), mk_prim(prim_subtract), the_global_environment);
        define_variable(mk_sym("/"), mk_prim(prim_divide), the_global_environment);
        define_variable(mk_sym("mod"), mk_prim(prim_mod), the_global_environment);

        // special forms
        define_variable(mk_sym("quote"), mk_syntax(syntax_quote), the_global_environment);
        define_variable(mk_sym("if"), mk_syntax(syntax_if), the_global_environment);
        define_variable(mk_sym("define"), mk_syntax(syntax_define), the_global_environment);
        define_variable(mk_sym("lambda"), mk_syntax(syntax_lambda), the_global_environment);
        //PRINT(the_global_environment);
    }

    return ;
}

void test_prim() {
    // (+ 1 2 3)
    struct object* exp = 
        cons(mk_sym("+"),
                cons(mk_integer(1),
                    cons(mk_integer(2), NULL)));
    print(exp);
    newline();
    print(eval(exp, the_global_environment));
    newline();
}

void test_special_forms() {
    /* ======= test define ========*/
    {
        printf("\n===========test define==============\n");
        // (define x 2)
        struct object* exp = 
            cons(mk_sym("define"),
                    cons(mk_sym("x"), 
                        cons(mk_integer(43), NULL)));
        print(exp);
        newline();
        print(eval(exp, the_global_environment));
        newline();

        // (define (square x) (* x x))
        struct object* params = cons(mk_sym("square"), cons(mk_sym("x"), NULL));
        struct object* body = cons(mk_sym("*"), cons(mk_sym("x"), cons(mk_sym("x"), NULL)));
        exp = cons(mk_sym("define"),
            cons(params, cons(body, NULL)));
        //print(exp);
        //newline();
        print(eval(exp, the_global_environment));
        newline();

        //newline();
        //print(the_global_environment);

        exp = cons(mk_sym("square"), cons(mk_integer(8), NULL));
        print(exp);
        newline();
        print(eval(exp, the_global_environment));
        printf("\n==================================\n");

    }

    /* ======= test quote ========*/
    {
        printf("\n===========test define==============\n");
        // (quote (1 2))
        struct object* exp = cons(mk_sym("quote"), cons(mk_integer(1), cons(mk_integer(2), NULL)));
        PRINT(exp);
        PRINT(eval(exp, the_global_environment));
        printf("\n==================================\n");
    }


    /* ======= test if ========*/
    {
        printf("\n===========test if==============\n");
        // (if (quote ()) true false)
        struct object* predicate = cons(mk_sym("quote"),
                cons(mk_sym("()"), NULL));
        struct object* consequent = mk_str("() is true");
        struct object* alternative = mk_str("() is not true");
        struct object* exp = list (4, mk_sym("if"), predicate, consequent, alternative);
        PRINT(exp);
        PRINT(eval(exp, the_global_environment));
        printf("\n==================================\n");
    }

    /* ======= test lambda ========*/
    {
        printf("\n===========test lambda==============\n");
        // (lambda (x) (* x x))
        struct object* params = cons(mk_sym("x"), NULL);
        struct object* body = list(3, mk_sym("*"), mk_sym("x"), mk_sym("x"));
        struct object* exp = list(3, mk_sym("lambda"), params, body);
        PRINT(exp);
        PRINT(eval(exp, the_global_environment));
        exp = list(2, exp, mk_integer(2));
        PRINT(exp);
        PRINT(eval(exp, the_global_environment));
        printf("\n==================================\n");
    }
}

void test_print() {
        struct object* o = NULL;
        o = mk_integer(100);
        print(o);
        newline();

        o = mk_str("hello world");
        print(o);
        newline();

        o = cons(mk_integer(100), NULL);
        print(o);
        newline();

        o = cons(mk_integer(1), 
                cons(mk_str("first"), NULL));
        struct object* b = cons(mk_integer(2),
                cons(mk_str("second"), NULL));
        print(cons(o, b));
        newline();

        o = cons(mk_integer(1), mk_str("one"));
        print(o);
        newline();
}

int main() {
    sparrow_init();

    //test_print();
    //test_prim();
    //test_special_forms();

    //load("test.l");
    while (true) {
        printf("sparrow>");
        //struct object* exp = read_exp(stdin);
        //print(exp);
        //newline();
        print(eval(read_exp(stdin), the_global_environment));
        newline();
    }
}

