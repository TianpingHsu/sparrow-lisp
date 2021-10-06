
(define (assert exp ans)
  (if (equal? exp ans) "pass" "FAIL"))

(define x 1)
(define y 2)
(assert (list x y 3) '(1 2 3))
(define (sum . list) (foldl + 0 list))
(assert (sum 1 2 3) 6)
(assert (odd? 2) #f)
(assert (even? 2) #t)
(assert (min 1 3 4  0 89) 0)
(assert (max 23 1 90 2 3) 90)
(assert (filter odd? (list 1 2 3 4 5 6)) '(1 3 5))
(assert (sort (list 7 6 9 1 0 19 21)) '(0 1 6 7 9 19 21))

