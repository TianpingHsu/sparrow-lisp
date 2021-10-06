# sparrow-lisp
*sparrow* is a scheme-like lisp interpreter implemented in C.  
the name *sparrow* comes from the idiom:  
> the __*sparrow*__ may be small but all its vital organs are there.  

I have been always wanted to write a LISP interpreter, especially when I was a college student.  
recently, I reread the book [_Structure and Interpretation of Computer Programs_](https://mitpress.mit.edu/sites/default/files/sicp/index.html) and I find that it's time to get my hands dirty.  


# quick start
to get read-eval-print-loop:  
>$make  
>$./sparrow  

to run the test code:  
>$make test  

and you'll get something like this:  
```
    ************************
    (assert (sum 1 2 3 4) 10)
    ==> "pass"
    ************************

    ************************
    (assert (odd? 2) #f)
    ==> "pass"
    ************************

    ************************
    (assert (even? 2) #t)
    ==> "pass"
    ************************

    ************************
    (assert (min 1 3 4 0 89) 0)
    ==> "pass"
    ************************

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
```


# supported forms
```scheme
;; define
(define <name> <val>)
(define (<name> <formal-paramerters>) <body>)

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
```

# not supported
1. macro system  
2. continuation  
3. anything else  

p.s. gc is not implemented, you can do it yourself, mark-and-sweep is easy.  


# useful materials
- [lis.py](https://norvig.com/lispy.html) and [lispy.py](https://norvig.com/lispy2.html) are written in Python by [Peter Norvig](http://norvig.com/). both are very easy to understand.  
- [this](https://mitpress.mit.edu/sites/default/files/sicp/code/ch4-mceval.scm) is the [SICP](https://mitpress.mit.edu/sites/default/files/sicp/index.html)'s metacircular evaluator.  
- the implementation of [microlisp](https://github.com/lazear/microlisp) takes the homoiconic approach outlined in SICP's metacircular evaluator.  
- [micro-lisp](https://github.com/carld/micro-lisp) tries to implement a small Lisp/Scheme language in as little C code as possible.  
- some test code comes from [yoctolisp](https://github.com/fragglet/yoctolisp).  
- [minilang.rkt](https://matt.might.net/articles/implementing-a-programming-language/) is written in [Racket](https://racket-lang.org/).  

