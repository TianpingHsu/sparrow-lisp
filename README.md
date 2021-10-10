# sparrow-lisp
*sparrow* is a scheme-like lisp interpreter implemented in C.  
the name *sparrow* comes from the idiom:  
> the __*sparrow*__ may be small but all its vital organs are there.  

I have been always wanted to write a LISP interpreter, especially when I was a college student.  
recently, I reread the book [_Structure and Interpretation of Computer Programs_](https://mitpress.mit.edu/sites/default/files/sicp/index.html) and I find that it's time to get my hands dirty.  


# quick start
to get the read-eval-print-loop:  
    > $make  
    > $./sparrow


to run [sicp's meta-circular evaluator](./res/mceval.scm) on sparrow:
    > $make mceval  
    > $./mceval


to run the test code:  
    > $make test  
    > $./test

and you'll get something like this:  
```
    ************************
    (assert (max 23 1 90 2 3) 90)
    ==> "pass"
    ************************

    ************************
    (assert (filter odd? (list 1 2 3 4 5 6)) (quote (1 3 5)))
    ==> "pass"
    ************************

    ************************
    (assert (map square (list 1 2 3 4 5)) (quote (1 4 9 16 25)))
    ==> "pass"
    ************************

    ************************
    (assert (sort (list 7 6 9 1 0 19 21)) (quote (0 1 6 7 9 19 21)))
    ==> "pass"
    ************************

    ************************
    (assert (fact 5) 120)
    ==> "pass"
    ************************

    ************************
    (assert (fib 8) 34)
    ==> "pass"
    ************************

    ************************
    (assert (eval (quote (sum 1 2 3))) 6)
    ==> "pass"
    ************************

    ************************
    (assert (apply * 1 2 3 (quote (4 5))) 120)
    ==> "pass"
    ************************
```


# supported forms
```scheme
;; define
(define <name> <val>)
(define (<name> <formal-paramerters>) <body>)
;; block definition and internal structure is also supported

;; lambda
(lambda (<formal-parameters>) <body>) ;; (lambda (x) (* x x))

;; variadic arguments
(define (<name> . <arglist>) <body>)  ;; (define (sum . l ) (apply + l))
(lambda <arglist> <body>)  ;; (lambda args (apply func (cons x args)))

;; if
(if <predicate> <consequent> <alternative>)

;; cond
(cond (<p1> <e1>)
      (<p2> <e2>)
      ...
      (<pn> <en>))


;; let
(let ((<var1> <exp1>)
      (<var2> <exp2>)
      (<var3> <exp3>)
      ...
      (<varn> <expn>))
  <body>)

;; set!
(set! <name> <new-value>)

;; begin
(begin
        <exp1>
        <exp2>
        ...
        <expn>)

;; etc.

```

# not supported
1. macro system  
2. continuation  

p.s. gc is not implemented on purpose, you can do it yourself, mark-and-sweep is easy.  


# useful materials
- [lis.py](https://norvig.com/lispy.html) and [lispy.py](https://norvig.com/lispy2.html) are written in Python by [Peter Norvig](http://norvig.com/). both are very easy to understand.  
- [this](https://mitpress.mit.edu/sites/default/files/sicp/code/ch4-mceval.scm) is the [SICP](https://mitpress.mit.edu/sites/default/files/sicp/index.html)'s metacircular evaluator.  
- the implementation of [microlisp](https://github.com/lazear/microlisp) takes the homoiconic approach outlined in SICP's metacircular evaluator.  
- [micro-lisp](https://github.com/carld/micro-lisp) tries to implement a small Lisp/Scheme language in as little C code as possible.  
- some test code comes from [yoctolisp](https://github.com/fragglet/yoctolisp).  
- [minilang.rkt](https://matt.might.net/articles/implementing-a-programming-language/) is written in [Racket](https://racket-lang.org/).  

