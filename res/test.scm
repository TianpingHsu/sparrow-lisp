(define (assert exp ans)
  (if (equal? exp ans) "pass" "FAIL"))

(define increase (curry + 1))
(assert (increase 10) 11)

(define (my-sum-v1 . l) (apply + l))
(define my-sum-v2
  (lambda l (apply + l)))

;; test closure (code comes from sicp)
(define balance 100)
(define (make-withdraw balance)
  (lambda (amount)
    (if (>= balance amount)
      (begin (set! balance (- balance amount))
             balance)
      "Insufficient funds")))
(define W1 (make-withdraw 100))
(define W2 (make-withdraw 100))
(assert (W1 50) 50)
(assert (W2 70) 30)
(assert (W2 40) "Insufficient funds")
(assert (W1 40) 10)

(define square (lambda (x) (* x x)))
(assert (square 8) 64)
(assert (list 1 2 3) '(1 2 3))
(assert (sum 1 2 3 4) 10)
(assert (my-sum-v1 1 2 3 4) 10)
(assert (my-sum-v2 1 2 3 4) 10)
(assert (odd? 2) #f)
(assert (even? 2) #t)
(assert (min 1 3 4  0 89) 0)
(assert (max 23 1 90 2 3) 90)
(assert (filter odd? (list 1 2 3 4 5 6)) '(1 3 5))
(assert (map square (list 1 2 3 4 5)) '(1 4 9 16 25))
(assert (sort (list 7 6 9 1 0 19 21)) '(0 1 6 7 9 19 21))
