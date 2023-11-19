-- Night time fire detection

function init()
  -- return 4 channels, RGBA
  return 4
end

function process()
  for x = 0, rgb_output:width() -1, 1 do
    for y = 0, rgb_output:height() -1, 1 do

      -- set variables for each calibrated channel with their respective names
      
      local cch3b = get_calibrated_value(3, x, y, true)
      local cch4 = get_calibrated_value(4, x, y, true)
      
      if cch3b > 305 then
        if cch4 > 280 then
          if (cch3b-cch4) > 10 then
            -- output as red and 100% opaque if the pixel passes the thresholds
            set_img_out(0, x, y, 1)
            set_img_out(3, x, y, 1)
          else
            -- if not, output as 0 and 100% transparent
            set_img_out(0, x, y, 0)
            set_img_out(3, x, y, 0)
          end
        else
          set_img_out(0, x, y, 0)
          set_img_out(3, x, y, 0)
        end
      else
        set_img_out(0, x, y, 0)
        set_img_out(3, x, y, 0)
      end
    end
    --set the progress bar accordingly
    set_progress(x, rgb_output:width())
  end
end
