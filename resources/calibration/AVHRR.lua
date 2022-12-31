-- AVHRR/3 Calibration

function calc_rad(channel, pos_y, px_val)
    local nlin = perLine_perChannel[pos_y][channel]["Ns"] + (perLine_perChannel[pos_y][channel]["Nbb"] - perLine_perChannel[pos_y][channel]["Ns"]) * (perLine_perChannel[pos_y][channel]["Spc"] - px_val) / (perLine_perChannel[pos_y][channel]["Spc"] - perLine_perChannel[pos_y][channel]["Blb"])
    out_rad = nlin + perChannel[channel]["b"][0] + perChannel[channel]["b"][1] * nlin + perChannel[channel]["b"][2] * nlin * nlin;
end

function init()
    perLine_perChannel = lua_vars["perLine_perChannel"]
    perChannel = lua_vars["perChannel"]
end

function compute(channel, pos_x, pos_y, px_val)
    if (channel < 3) then
        local crossover = (perChannel[channel]["int_hi"] - perChannel[channel]["int_lo"])/(perChannel[channel]["slope_lo"] - perChannel[channel]["slope_hi"])
        if (px_val <= crossover) then
            return perChannel[channel]["slope_lo"] * px_val + perChannel[channel]["int_lo"]
        else
            return perChannel[channel]["slope_hi"] * px_val + perChannel[channel]["int_hi"]
        end
    else
        if pcall(calc_rad, channel, pos_y, px_val) then
            return out_rad
        else
            return 0
        end
    end
end
