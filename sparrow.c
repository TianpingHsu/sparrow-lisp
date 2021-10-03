#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct object* (*primitive_t)(struct object* args);
typedef struct object* (*syntax_t)(struct object*, struct object* );

struct object {
    enum {
        NUMBER, SYMBOL, STRING, LIST, PROCEDURE, PRIMITIVE, ENVIRONMENT, SYNTAX
    } type;
    union {
        int64_t interger;
        char* s;  // STRING or SYMBOL
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

extern char* strdup(const char*);
struct object* mk_str(const char* s) {
    struct object* o = NULL;
    if (s) {
        o = mk_obj(STRING);
        o->s = strdup(s);
    }
    return o;
}

uint64_t hash(const char* s) {
    // check [here](http://www.cse.yorku.ca/~oz/hash.html)
    unsigned long hash = 5381;
    int c;
    while ((c = *s++)) {
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash % g_sym_table.size;  // collision is possible
    /*
        this one is not good, 
        it happens that "#f" and "if" has same hash value when HTABLE_SIZE is 8191

        uint64_t h = 0;
        uint8_t *u = (uint8_t *)s;
        while (*u) {
         h = (h * 256 + *u) % HTABLE_SIZE;
         u++;
        }
        return h;
    */
}

struct object* mk_sym(const char* s) {
    uint64_t k = hash(s);
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

struct object* mk_integer(int64_t x) {
    struct object* o = mk_obj(NUMBER);
    o->interger = x;
    return o;
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
            case NUMBER:
                printf("%ld", o->interger);
                break;
            case SYMBOL:
                printf("%s", o->s);
                break;
            case STRING:
                printf("\"%s\"", o->s);
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
    return mk_integer(product );
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
    return eval(cdr(exp), env);
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
                print(body);
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
// TODO




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
        struct object* e_true = mk_integer(1);
        struct object* e_false = mk_integer(0);
        define_variable(mk_sym("#t"), e_true, the_global_environment);
        define_variable(mk_sym("true"), e_true, the_global_environment);
        define_variable(mk_sym("#f"), e_false, the_global_environment);
        define_variable(mk_sym("false"), e_false, the_global_environment);

        // primitives
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

        //print(the_global_environment);
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
        //print(exp);
        //newline();
        //print(eval(exp, the_global_environment));
        //newline();

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


    /* ======= test if ========*/
    

    /* ======= test lambda ========*/

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
    test_special_forms();
}

