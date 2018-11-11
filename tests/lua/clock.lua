require "bootstrap-tests"

test.clock.api = function(clock_type)
    local clock =  effil.clock[clock_type];
    local sec = clock.s();
    local ms = clock.ms();
    local ns = clock.ns();
    test.assert(sec * 1000 <= ms)
    test.assert(ms * 1000 <= ns)
end

test.clock.api "system"
test.clock.api "steady"
