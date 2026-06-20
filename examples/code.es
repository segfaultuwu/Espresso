def blink
    gpio 2 1
    sleep delay
    gpio 2 0
    sleep delay
end

delay = 500

while 1==1
    blink
end
