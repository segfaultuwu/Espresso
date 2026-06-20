def countdown(n)
    print n
    if n >= 1
        m = n - 1
        countdown(m)
    end
end
countdown(3)
