

function init()
    return 1 -- Number of channels to output
end

function process()
    local d1 = 0.754
    local d2 = -2.265
    local adj = 0.003 

    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do
            local ch23_8 = get_calibrated_value(0, x, y, true)
            local ch31_4 = get_calibrated_value(1, x, y, true)
            local centerPixel = x - rgb_output:width() / 2
            local theta = (centerPixel / rgb_output:width() * 98) * math.pi / 180
            local d0 = 8.24 - 2.622*math.cos(theta) + 1.846 * (math.cos(theta))^2

            local result = math.cos(theta)*(d0+d1*math.log(285 - ch23_8)+c2*math.log(285-ch31_4))+c3
            local output = result / 300
            set_img_out(0, x, y, output)
        end

        set_progress(x, rgb_output:width())
    end
end

