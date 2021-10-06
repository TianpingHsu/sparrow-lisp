# sparrow-lisp
this is a scheme-like lisp interpreter implemented in C.  
**sparrow** comes from the idiom:  
> the sparrow may be small but all its vital organs are there.  

I have been always wanted to write a LISP interpreter, especially when I was a college student.  
recently, I reread the book [_Structure and Interpretation of Computer Programs_](https://mitpress.mit.edu/sites/default/files/sicp/index.html) and I find that it's time to get my hands dirty.  

p.s. gc is not implemented, you can do it yourself, mark-and-sweep is easy.  


# supported forms
```lisp
;; define
(define <name> <val>)
(define (<name> <formal-paramerters>) <body>)

;; if
(if <predicate> <consequent> <alternative>)

;; cond
(cond (<p1> <e1>)
      (<p2> <e2>)
      ...
      (<pn> <en>))

;; lambda
(lambda (<formal-parameters>) <body>)


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



# useful materials
- [lis.py](https://norvig.com/lispy.html) and [lispy.py](https://norvig.com/lispy2.html) are written in Python by [Peter Norvig](http://norvig.com/). both are very easy to understand.  
- this is the [SICP](https://mitpress.mit.edu/sites/default/files/sicp/index.html)'s [metacircular evaluator](https://mitpress.mit.edu/sites/default/files/sicp/code/ch4-mceval.scm).  
- the implementation of [microlisp](https://github.com/lazear/microlisp) takes the homoiconic approach outlined in SICP's metacircular evaluator.  
- [micro-lisp](https://github.com/carld/micro-lisp) tries to implement a small Lisp/Scheme language in as little C code as possible.  
- some test code comes from [yoctolisp](https://github.com/fragglet/yoctolisp).  
- [minilang.rkt](https://matt.might.net/articles/implementing-a-programming-language/) is written in [Racket](https://racket-lang.org/).  

