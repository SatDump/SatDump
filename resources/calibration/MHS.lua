-- MHS Calibration

function calc_rad(channel, pos_y, px_val)
    out_rad = perLine_perChannel[pos_y][channel]["a0"] + perLine_perChannel[pos_y][channel]["a1"] * px_val +
        perLine_perChannel[pos_y][channel]["a2"] * px_val * px_val;
end

function init()
    perLine_perChannel = lua_vars["perLine_perChannel"]
end

function compute(channel, pos_x, pos_y, px_val)

    if pcall(calc_rad, channel, pos_y, px_val) then
        return out_rad
    else
        return 0
    end

end
