-- 10.8, 3.9 um fog BTD

function init()
    --return 3 channels, RGB
    return 3
end

function process()
    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do

            --get channels
            
            get_channel_values(x, y)
            local cch3b = get_channel_value(0) * 500
            local cch4 = get_channel_value(1) * 500
            local btd = cch4 - cch3b
            
            --return RGB 0=R 1=G 2=B
            
            set_img_out(0, x, y, btd)
            set_img_out(1, x, y, btd)
            set_img_out(2, x, y, btd)
            
        end
        --set the progress bar accordingly
        set_progress(x, rgb_output:width())
    end
end
