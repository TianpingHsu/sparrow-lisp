


(define true #t)
(define false #f)
(define else #t)

(define (list . values) values)

(define (foldl func acc list)
  (if (null? list)
    acc
    (foldl func
           (func acc (car list))
           (cdr list))))

(define (foldr func acc list)
  (if (null? list)
    acc
    (func (car list)
          (foldr func acc (cdr list)))))


(define (map callback list)
  (if (null? list)
    '()
    (cons (callback (car list))
          (map callback (cdr list)))))

(define (filter callback list)
  (if (null? list)
    '()
    (if (callback (car list))
      (cons (car list)
            (filter callback (cdr list)))
      (filter callback (cdr list)))))

(define (for-each callback list)
  (if (null? list)
    '()
    (begin (callback (car list))
           (for-each callback (cdr list)))))

(define (reduce func list)
  (foldl func (car list) (cdr list)))

(define (curry func x)
  (lambda args (apply func (cons x args))))

(define (compose f g)
  (lambda args (f (apply g args))))

(define (length list)
  (foldl (lambda (x) (+ x 1)) 0 list))

(define (sum . list) (foldl + 0 list))
(define (product . list) (foldl * 1 list))

(define (>= x y) (not (< x y)))

(define (> x y) 
  (cond ((= x y) #f) 
        ((< x y) #f)
        (else #t)))

(define (append x y)
  (if (null? x)
    y
    (cons (car x) (append (cdr x) y))))

(define (sort l)
  (if (null? l)
    l
    (let ((pivot (car l)))
      (let ((left (filter (lambda (x) (< x pivot)) (cdr l)))
            (right (filter (lambda (x) (>= x pivot)) (cdr l))))
        (append (sort left)
                (append (list pivot)
                        (sort right)))))))

(define (min first . list)
  (foldl (lambda (x y) (if (< x y) x y))
         first
         list))

(define (max first . list)
  (foldl (lambda (x y) (if (> x y) x y))
         first
         list))

(define (id x) x)

(define (odd? x) 
  (if (= (mod x 2) 1) #t #f))
(define (even? x) (not (odd? x)))
