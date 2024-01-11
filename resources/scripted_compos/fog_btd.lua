-- 10.8, 3.9 um fog BTD

function init()
    --create an empty image for the LUT
    img_lut = image8.new()
    --load the LUT
    img_lut:load_png(get_resource_path("lut/fog_btd.png"))
    --return 3 channels, RGB
    return 3
end

function process()
    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do

            --get channels

            get_channel_values(x, y)
            local cch3b = get_channel_value(0) * 1000
            local cch4 = get_channel_value(1) * 1000
            local btd = cch4 - cch3b
            print(btd)

            --range convert from -1 -> 1 to 0 -> 256
            local btd_lutval = (((btd-(-15))*256) / (15-(-15)))

            --sanity checks (noise)
            if btd_lutval < 0 then
                btd_lutval = 0
            end
            
            if btd_lutval > 255 then
                btd_lutval = 255
            end


            --don't forget to create a table!
            local lut_result = {}
    
            --send the ndvi_lutval to the LUT, retrieve it back in the table as 3 R,G,B channels
            lut_result[0] = img_lut:get(0 * img_lut:height() * img_lut:width() + btd_lutval * img_lut:width() + btd_lutval) / 255.0
            lut_result[1] = img_lut:get(1 * img_lut:height() * img_lut:width() + btd_lutval * img_lut:width() + btd_lutval) / 255.0
            lut_result[2] = img_lut:get(2 * img_lut:height() * img_lut:width() + btd_lutval * img_lut:width() + btd_lutval) / 255.0
            
            

            --return RGB 0=R 1=G 2=B
            set_img_out(0, x, y, lut_result[0])
            set_img_out(1, x, y, lut_result[1])
            set_img_out(2, x, y, lut_result[2])
            
        end
        --set the progress bar accordingly
        set_progress(x, rgb_output:width())
    end
end