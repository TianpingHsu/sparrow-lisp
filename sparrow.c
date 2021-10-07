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
        int64_t integer;
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
static struct object* g_true = NULL;
static struct object* g_false = NULL;  // the only 'false'
static struct object* g_dummy = (struct object*)(-1);  // dummy obj
#define newline() putchar('\n')
#define PRINT(exp) do { \
    newline(); \
    print(exp); \
    newline(); \
} while(0)
static const char* types_str[] = \
{"boolean", "number", "symbol", "string", "port", "list", "procedure", "primitive","environmen", "syntax"};
#define REQUIRE(exp, TYPE) do { \
    if ((!exp && TYPE != LIST) || (exp && exp->type != TYPE)) { \
        printf("require type: %s, but exp has type: %s\n", types_str[TYPE], \
                exp ? types_str[exp->type] :  types_str[LIST]);\
        assert(0); \
    } \
} while(0)

struct object* eval(struct object* exp, struct object* env);
void print(struct object* o);

/*========================================================
 * constructors
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
    o->integer = x;
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
int len(struct object* l) {
    if (!l) return 0;
    REQUIRE(l, LIST);
    return 1 + len(cdr(l));
}

void print(struct object* o) {
    if (!o) {
        printf("()");
    } else {
        switch (o->type) {
            case BOOLEAN:
                printf("%s", o->b ? "#t" : "#f");
                break;
            case NUMBER:
                printf("%ld", o->integer);
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
                printf("<BUILTIN-PRIMITIVE>");
                break;
            case PROCEDURE:
                printf("<COMPOUND-PROCEDURE>");
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
                        printf("\n");
                        vars = cdr(vars);
                        vals = cdr(vals);
                    }
                    o = o->parent;
                    if (o) {
                        printf("----parent------>\n");
                    } else {
                        printf("----end of environment------\n");
                    }
                }
                break;
            case SYNTAX:
                printf("SPECIAL-FORM");
                break;
            default:
                printf("DEFAULT");
                break;
        }
    }
}

/*========================================================
 * environment handling
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
    return g_dummy;  // unbound variable
}

struct object* set_variable(struct object* var, struct object* val, struct object* env) {
    REQUIRE(var, SYMBOL);
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

// define variable in *current* frame
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

/*========================================================
 * builtins: primitives and syntax
 * =======================================================*/

// helper
struct object* reverse(struct object* l, struct object* base) {
    if (!l) return base;
    return reverse(cdr(l), cons(car(l), base));
}

// helper
struct object* append(struct object* x, struct object* y) {
    if (!x) return y;  // both x and y are LIST
    return cons(car(x), append(cdr(x), y));
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

bool is_equal(struct object *x, struct object *y) {
     if (!x || !y) return x == y;
     if (x->type != y->type) return false;
     switch (x->type) {
     case LIST:
         return is_equal(car(x), car(y)) && is_equal(cdr(x), cdr(x));
     case NUMBER:
         return x->integer == y->integer;
     case STRING:
         return !strcmp(x->s, y->s);
     default:
         return x == y;
     }
}

struct object* prim_eq(struct object* exp) {
    // (equal x y)
    exp = cdr(exp);
    struct object* x = car(exp);
    struct object* y = cadr(exp);
    return is_equal(x, y) ? g_true : g_false;
}
struct object* prim_is_pair(struct object* exp) {
    // (pair? exp)
    struct object* o = cadr(exp);
    return o && o->type == LIST && cdr(o) != NULL ? g_true : g_false;
}

struct object* prim_is_symbol(struct object* exp) {
    // (symbol? exp)
    struct object* o = cadr(exp);
    return o && o->type == SYMBOL ? g_true : g_false;
}

struct object* prim_isnull(struct object* exp) {
    // (null? l)
    struct object* l = cadr(exp);
    return l == NULL ? g_true : g_false;
}

struct object* prim_add(struct object* l) {
    // (+ x ...)
    l = cdr(l);
    int64_t sum = car(l)->integer;
    while ((l = cdr(l))) {
        sum += car(l)->integer;
    }
    return mk_integer(sum);
}

struct object* prim_multiply(struct object* l) {
    // (* x ...)
    l = cdr(l);
    int64_t product = car(l)->integer;
    while ((l = cdr(l))) {
        product *= car(l)->integer;
    }
    return mk_integer(product);
}

struct object* prim_subtract(struct object* l) {
    // (- x ...)
    l = cdr(l);
    int64_t sum = car(l)->integer;
    while ((l = cdr(l))) {
        sum -= car(l)->integer;
    }
    return mk_integer(sum);
}

struct object* prim_divide(struct object* l) {
    // (- x y)
    l = cdr(l);
    int64_t quot = car(l)->integer;
    quot /= cadr(l)->integer;
    return mk_integer(quot);
}

struct object* prim_mod(struct object* l) {
    // (mod x y)
    l = cdr(l);
    int64_t quot = car(l)->integer;
    quot %= cadr(l)->integer;
    return mk_integer(quot);
}

struct object* prim_num_eq(struct object* exp) {
    // (= x y)
    struct object* args = cdr(exp);
    struct object* x = car(args);
    struct object* y = cadr(args);
    REQUIRE(x, NUMBER);
    REQUIRE(y, NUMBER);
    return x->integer == y->integer ? g_true : g_false;
}

struct object* prim_num_lt(struct object* exp) {
    // (< x y)
    struct object* args = cdr(exp);
    struct object* x = car(args);
    struct object* y = cadr(args);
    REQUIRE(x, NUMBER);
    REQUIRE(y, NUMBER);
    return x->integer < y->integer ? g_true : g_false;
}

struct object* prim_not(struct object* exp) {
    // (not x)
    return cadr(exp) == g_false ? g_true : g_false;
}

struct object* prim_display(struct object* exp) {
    // (display x)
    print(cadr(exp));
    printf("\n");
    return NULL;
}

struct object* prim_eval(struct object* exp) {
    // (eval exp)
    return eval(cadr(exp), the_global_environment);
}

struct object* syntax_apply(struct object* exp, struct object* env) {
    // (apply func x y ... l)  ;; l must be LIST
    //PRINT(exp);
    struct object* func = cadr(exp);
    struct object* args = cddr(exp);
    struct object* l = NULL;
    do {
        struct object* arg = eval(car(args), env);
        if (arg->type == LIST && cdr(args) == NULL) {
            // last arg must be LIST
            args = arg;
            break;
        }
        l = cons(eval(arg, env), l);
        args = cdr(args);
    } while (true);
    l = reverse(l, NULL);
    args = append(l, args);
    return eval(cons(func, args), env);
}

struct object* load(struct object* module);


struct object* syntax_if(struct object* exp, struct object* env) {
    // (if predicate consequent alternative)
    struct object* predicate = cadr(exp);
    struct object* ret = eval(predicate, env);
    if (ret == g_false){
        struct object* alternative = cadr(cddr(exp));
        return eval(alternative, env);
    } else {
        struct object* consequent = caddr(exp);
        return eval(consequent, env);
    }
}

struct object* syntax_quote(struct object* exp, struct object* env) {
    // (quote <datum>)
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
    if (params->type != LIST && params->type == SYMBOL) {  // variadic
        params = cons(mk_sym("."), cons(params, NULL));
    }
    struct object* closure = mk_procedure(params, body, env);
    return closure;
}

struct object* syntax_cond(struct object* exp, struct object* env) {
    /*
     * (cond (<p1> <e1>)
     *       (<p2> <e2>)
     *       ...
     *       (<pn> <en>))
     */
    struct object* clauses = cdr(exp);
    while (clauses) {
        struct object* clause = car(clauses);
        struct object* predicate = car(clause);
        struct object* test = eval(predicate, env);
        //PRINT(clause);
        //PRINT(predicate);
        //PRINT(test);
        if (test != g_false) {
            return eval(cadr(clause), env); 
        } else {
            clauses = cdr(clauses);
        }
    }
    return NULL;
}

struct object* syntax_begin(struct object* exp, struct object* env) {
    /*
     * (begin <e1> <e2> ... <en>)
     */
    struct object* actions = cdr(exp);
    struct object* ret = NULL;
    while (actions) {
        ret = eval(car(actions), env);
        actions = cdr(actions);
    }
    return ret;
}

struct object* syntax_let(struct object* let_exp, struct object* env) {
    /*
     * (let ((<var1> <exp1>) ... (<varn> <expn>)) <body>)
     * <=>
     * ((lambda (<var1> ... <varn>) <body>) <exp1> ... <expn>)
     */
    struct object* pairs = cadr(let_exp);
    struct object* vars = NULL;
    struct object* exps = NULL;
    while (pairs) {
        struct object* pair = car(pairs);
        vars = cons(car(pair), vars);
        exps = cons(cadr(pair), exps);
        pairs = cdr(pairs);
    }
    vars = reverse(vars, NULL);
    exps = reverse(exps, NULL);
    struct object* body = cddr(let_exp);
    struct object* lambda = 
        cons(mk_sym("lambda"), cons(vars, body));
    struct object* transformed = cons(lambda, exps);
#ifdef DEBUG
    //PRINT(let_exp);
    //PRINT(transformed);
#endif
    return eval(transformed, env);
}

struct object* syntax_set(struct object* exp, struct object* env) {
    // (set! x y)
    struct object* var = cadr(exp);  // symbol
    struct object* val = eval(caddr(exp), env);
    return set_variable(var, val, env);
}

struct object* syntax_set_car(struct object* exp, struct object* env) {
    // (set-car! x y)
    struct object* var = cadr(exp);
    REQUIRE(var, LIST);
    struct object* val = caddr(exp);
    var->car = val;
    return val;
}

struct object* syntax_set_cdr(struct object* exp, struct object* env) {
    // (set-cdr! x y)
    struct object* var = cadr(exp);
    REQUIRE(var, LIST);
    struct object* val = caddr(exp);
    var->cdr = val;
    return val;
}

struct object* syntax_not_supported(struct object* exp, struct object* env) {
    printf("SYNTAX NOT SUPPORTED:\n");
    print(exp);
    printf("\n");
    return NULL;
}


/*========================================================
 * evaluator
 * =======================================================*/
struct object* eval_args(struct object* args, struct object* env) {
    struct object* l = NULL;
    while (args) {
        struct object* arg = car(args);
        l = cons(eval(arg, env), l);
        args = cdr(args);
    }
    return reverse(l, NULL);
}

struct object* eval_list(struct object* exp, struct object* env) {
    struct object* func = eval(car(exp), env);  // eval operator
    switch (func->type) {
        case SYNTAX:  // special forms
                return (func->syntax)(exp, env);
        case PRIMITIVE:
                {
                    struct object* args = eval_args(cdr(exp), env);
                    exp = cons(func, args);
                    //printf("PRIMITIVE:");
                    //print(exp);
                    //newline();
                    return (func->primitive)(exp);
                }
        case PROCEDURE:
            {
                // (func <args>)
                // eval operands
                struct object* params = func->params;
                struct object* args = cdr(exp);
                struct object* new_env = mk_env(func->env);
                //PRINT(params);
                //PRINT(args);
                while (params && args) {
                    struct object* param = car(params);
                    if (param == mk_sym(".")) {  // varidic args
                        struct object* l = eval_args(args, env);
                        define_variable(cadr(params), l, new_env);
                        break;
                    }
                    define_variable(param, eval(car(args), env), new_env);
                    params = cdr(params);
                    args = cdr(args);
                }
                // apply
                struct object* body = func->body;
                //printf("=========== func body =========");
                //PRINT(body);
                //printf("===========\n");
                //printf("func new_env:");
                //PRINT(new_env);
                return eval(body, new_env);
            }
            break;
        default:
            print(car(exp));
            printf(" has type: %s, which is not appliable!\n", types_str[func->type]);
            abort();
            break;
    }
    return NULL;
}

struct object* eval(struct object* exp, struct object* env) {
    if (!exp) return NULL;
    switch (exp->type) {
        case NUMBER:
        case STRING:
        case BOOLEAN:
            return exp;
        case SYMBOL:
            {
                struct object* val = lookup_variable(exp, env);
                if (val == g_dummy) {
                    printf("Unbound Symbol: %s\n", exp->s) ;
                    abort();
                }
                return val;
            }
        case LIST:
            return eval_list(exp, env);
           break;
        default:
            return NULL;
    }
}

/*========================================================
 * parser
 * =======================================================*/
int peek(FILE* fp) { return ungetc(getc(fp), fp); }
bool is_space(int c) { return (c == ' ' || c == '\n' || c == '\r' || c == '\t'); }
struct object* read_exp(FILE* fp) {
    /*
     * (struct object*)(-1):
     * stands for both for 'EOF' and 'end of list'
     *
     * (struct object*)NULL:
     * stands for 'empty list'
     */
    static char SYMBOLS[] = "~!@#$%^&*_-+\\:,.<>|{}[]?=/";
    int c;
    for (;;) {
        if ((c = getc(fp)) == EOF) return g_dummy;
        if (is_space(c)) continue;

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
            while ((c = getc(fp)) != EOF) {
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

        if (c == '\'') {  // read quote exp
            struct object* quoted_exp = read_exp(fp);
            return cons(mk_sym("quote"), cons(quoted_exp, NULL));
        }

        if (isdigit(c) || ((c == '-') && isdigit(peek(fp)))) {  // read number
            int sign = (c == '-') ? -1 : 1;
            int sum = (sign == -1) ? 0 : (c - '0');
            while (isdigit(peek(fp))) {
                sum = sum * 10 + (getc(fp) - '0');
            }
            return mk_integer(sign * sum);
        }

        if (c == '(' && peek(fp) == ')') {  // read empty list
            getc(fp); return NULL;
        }
        if (c == '(') {  // read list
            struct object* l = NULL;
            while (true) {
                struct object* o = read_exp(fp);
                if (o == g_dummy) break;
                l = cons(o, l);
            }
            return reverse(l, NULL);
        }
        if (c == ')') {return g_dummy;  /*end of list*/}

        if (c == '#') {  // true or false
            if (peek(fp) == 't') {getc(fp);return g_true;}
            if (peek(fp) == 'f') {getc(fp);return g_false;}
        }

        if (isalpha(c) || strchr(SYMBOLS, c)) {  // read symbol
            char buf[128];
            buf[0] = c;
            int i = 1;
            while (isalnum(peek(fp)) || strchr(SYMBOLS, peek(fp))) {
                if (i >= 128)
                    printf("Symbol name too long - max length 128 characters");
                buf[i++] = getc(fp);
            }
            buf[i] = '\0';
            return mk_sym(buf);
        }
    }
}

/*========================================================
 * initialization
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
        g_true = mk_bool(true);
        g_false = mk_bool(false);
        define_variable(mk_sym("#t"), g_true, the_global_environment);
        define_variable(mk_sym("#f"), g_false, the_global_environment);
        define_variable(mk_sym("()"), NULL, the_global_environment);

        // primitives
        define_variable(mk_sym("cons"), mk_prim(prim_cons), the_global_environment);
        define_variable(mk_sym("car"), mk_prim(prim_car), the_global_environment);
        define_variable(mk_sym("cdr"), mk_prim(prim_cdr), the_global_environment);
        define_variable(mk_sym("equal?"), mk_prim(prim_eq), the_global_environment);
        define_variable(mk_sym("pair?"), mk_prim(prim_is_pair), the_global_environment);
        define_variable(mk_sym("symbol?"), mk_prim(prim_is_symbol), the_global_environment);
        define_variable(mk_sym("null?"), mk_prim(prim_isnull), the_global_environment);
        define_variable(mk_sym("not"), mk_prim(prim_not), the_global_environment);
        define_variable(mk_sym("+"), mk_prim(prim_add), the_global_environment);
        define_variable(mk_sym("*"), mk_prim(prim_multiply), the_global_environment);
        define_variable(mk_sym("-"), mk_prim(prim_subtract), the_global_environment);
        define_variable(mk_sym("/"), mk_prim(prim_divide), the_global_environment);
        define_variable(mk_sym("mod"), mk_prim(prim_mod), the_global_environment);
        define_variable(mk_sym("="), mk_prim(prim_num_eq), the_global_environment);
        define_variable(mk_sym("<"), mk_prim(prim_num_lt), the_global_environment);
        define_variable(mk_sym("load"), mk_prim(load), the_global_environment);
        define_variable(mk_sym("display"), mk_prim(prim_display), the_global_environment);
        define_variable(mk_sym("eval"), mk_prim(prim_eval), the_global_environment);

        // special forms
        define_variable(mk_sym("quote"), mk_syntax(syntax_quote), the_global_environment);
        define_variable(mk_sym("if"), mk_syntax(syntax_if), the_global_environment);
        define_variable(mk_sym("define"), mk_syntax(syntax_define), the_global_environment);
        define_variable(mk_sym("lambda"), mk_syntax(syntax_lambda), the_global_environment);
        define_variable(mk_sym("cond"), mk_syntax(syntax_cond), the_global_environment);
        define_variable(mk_sym("begin"), mk_syntax(syntax_begin), the_global_environment);
        define_variable(mk_sym("let"), mk_syntax(syntax_let), the_global_environment);
        define_variable(mk_sym("set!"), mk_syntax(syntax_set), the_global_environment);
        define_variable(mk_sym("set-car!"), mk_syntax(syntax_set_car), the_global_environment);
        define_variable(mk_sym("set-cdr!"), mk_syntax(syntax_set_cdr), the_global_environment);
        define_variable(mk_sym("apply"), mk_syntax(syntax_apply), the_global_environment);

#ifdef DEBUG
        printf("the global environment ==>\n");
        PRINT(the_global_environment);
#endif
    }

    return ;
}

struct object* load(struct object* module) {
    REQUIRE(module, STRING);
    const char* filename = module->s;
    FILE* fp = fopen(filename, "r");
    if (!fp) {printf("load failed!\n"); return NULL;}
    struct object* val = NULL;
    while (true) {
        struct object* exp = read_exp(fp);
        if (exp == g_dummy) break;
        val = eval(exp, the_global_environment);
#ifdef DEBUG
        printf("************************\n");
        print(exp);
        printf("\n==> ");
        print(val);
        printf("\n************************\n\n");
#endif
    }
    fclose(fp);
    return val;
}

int main() {
    sparrow_init();

    load(mk_str("./res/lib.scm"));
#ifdef DEBUG
    struct object* module = mk_str("./res/test.scm");
    load(module);
    return 0;
#endif

    while (true) {
        printf("sparrow>");
        print(eval(read_exp(stdin), the_global_environment));
        newline();
        newline();
        if (peek(stdin) == EOF) break;
    }
}

