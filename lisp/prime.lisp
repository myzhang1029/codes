; Prime number related little functions - newlisp

(define (_isprime a k)
  (if (> k (sqrt a))
    true
    (if (= 0 (mod a k))
      nil
      (_isprime a (+ k 1)))))

(define (isprime a)
  (if (< a 2)
    nil
    (if (= a 2)
      true
      (_isprime a 2))))

(define (_primebetween i x l)
  (if (>= i x)
    l
    (_primebetween (+ i 1) x (if (isprime i)
                               (append l (list i))
                               l))))

(define (primebetween i x)
  (_primebetween i x '()))

