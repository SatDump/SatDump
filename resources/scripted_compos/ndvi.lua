-- NDVI

function init()
    --create an empty image for the LUT
    img_lut = image_t.new()
    --load the LUT
    image_load_png(img_lut, get_resource_path("lut/NDVI.png"))
    --return 3 channels, RGB
    return 3
end

function process()
    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do

            --get channels from satdump.json

            get_channel_values(x, y)
            local cch1 = get_channel_value(0)
            local cch2 = get_channel_value(1)            
            --perform NDVI
            local ndvi = (cch2-cch1)/(cch2+cch1)

            --range convert from -1 -> 1 to 0 -> 256
            local ndvi_lutval = (((ndvi-(-1))*256) / (1-(-1)))

            --sanity checks (noise)
            if ndvi_lutval < 0 then
                ndvi_lutval = 0
            end
            
            if ndvi_lutval > 255 then
                ndvi_lutval = 255
            end


            --don't forget to create a table!
            local lut_result = {}
    
            --send the ndvi_lutval to the LUT, retrieve it back in the table as 3 R,G,B channels
            lut_result[0] = img_lut:get(0 * img_lut:height() * img_lut:width() + ndvi_lutval * img_lut:width() + ndvi_lutval) / 255.0
            lut_result[1] = img_lut:get(1 * img_lut:height() * img_lut:width() + ndvi_lutval * img_lut:width() + ndvi_lutval) / 255.0
            lut_result[2] = img_lut:get(2 * img_lut:height() * img_lut:width() + ndvi_lutval * img_lut:width() + ndvi_lutval) / 255.0
            
            

            --return RGB 0=R 1=G 2=B
            set_img_out(0, x, y, lut_result[0])
            set_img_out(1, x, y, lut_result[1])
            set_img_out(2, x, y, lut_result[2])
            
        end
        --set the progress bar accordingly
        set_progress(x, rgb_output:width())
    end
end
