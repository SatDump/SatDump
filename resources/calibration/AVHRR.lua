-- AVHRR/3 Calibration

function init()

end

function compute(channel, pos_x, pos_y, px_val)
    return 101 + pos_x;
end
