<div align="center">

# ☕ Espresso VM

A tiny embedded scripting language + bytecode VM for ESP32

![status](https://img.shields.io/badge/status-experimental-red)
![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v6.2-blue)
![language](https://img.shields.io/badge/language-C11-orange)

### Minimal scripting runtime for microcontrollers

### Note: this language is heavily inspired by Ruby

</div>

## Example

```ruby
while 1==1
    gpio(2,1)
    sleep(1000)
    gpio(2,0)
    sleep(1000)
end
```
