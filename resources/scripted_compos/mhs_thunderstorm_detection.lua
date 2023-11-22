-- SIGNIFICANT THUNDERSTORMS DETECTION USING MHS

--implement a rounding function for convenience
function round(num, dp)
    local mult = 10^(dp or 0)
    return math.floor(num * mult + 0.5)/mult
end

--return 3 RGB channels and alpha channel
function init()
    return 4
end

function process()
    --get the average of the BTs of the entire swath
    local cch2_sum = 0
    local cch5_sum = 0
    local count = 0

    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do

            --get channels from satdump.json for the current position
            get_channel_values(x, y)
            
            
            cch2_sum = cch2_sum + get_channel_value(0) 
            cch5_sum = cch5_sum + get_channel_value(1)
            count = count + 1
        end
    end

    --average of the whole image
    local cch2_avg = cch2_sum / count
    local cch5_avg = cch5_sum / count
    print("157 GHz Average BT = " .. cch2_avg * 1000 .. " K")
    print("189 GHz 3 Average BT = " .. cch5_avg * 1000 .. " K")

    local cch2_thresh = cch2_avg - 0.049
    local cch5_thresh = cch5_avg - 0.055

    print("157 GHz threshold = " .. cch2_thresh * 1000 .. " K")
    print("189 GHz 3 threshold = " .. cch5_thresh * 1000 .. " K")

    -- Now actually produce the image
    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do

            --re-get channels from satdump.json
            get_channel_values(x, y)
            
            
            local cch2 = get_channel_value(0)
            local cch5 = get_channel_value(1)

            --First stage of detection using channel 2
            if round(cch2, 3) < cch2_thresh then
                --second confirmation stage using channel 5
                if round(cch5, 3) < cch5_thresh then
                    --set red pixel and opacity to 100%
                    set_img_out(0, x, y, 1)
                    set_img_out(3, x, y, 1)
                else
                    --set yellow pixel (green + red) and opacity to 100%
                    set_img_out(0, x, y, 1)
                    set_img_out(1, x, y, 1)
                    set_img_out(3, x, y, 1)
                end
            else
                --set alpha channel to min
                set_img_out(3, x, y, 0)
            end    
        end
        --set the progress bar accordingly
        set_progress(x, rgb_output:width())
    end
end
