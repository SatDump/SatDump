-- Transparent snow cover using MHS

function init()
    if not has_sat_proj() then
        lerror("This composite requires projection info")
        return 0
    end

	sat_proj = get_sat_proj()
	img_landmask = image8.new()
	img_landmask:load_jpeg(get_resource_path("maps/landmask.jpg"))
	img_landmask:linear_invert()
	equ_proj = EquirectangularProj.new()
	equ_proj:init(img_landmask:width(), img_landmask:height(), -180, 90, 180, -90)
	return 4
end

function process()
	pos = geodetic_coords_t.new()

	for x = 0, rgb_output:width() -1, 1 do
		for y = 0, rgb_output:height() -1, 1 do

			get_channel_values(x, y)

			if not sat_proj:get_position(x, y, pos) then
				x2, y2 = equ_proj:forward(pos.lon, pos.lat)

				mappos = y2 * img_landmask:width() + x2

				mval = img_landmask:get(img_landmask:width() * img_landmask:height() + mappos) / 255.0

				if mval < 0.1 then
					local cch1 = get_channel_value(0)
					local cch2 = get_channel_value(1)

					local redChannel = 1-cch1

					if cch2-cch1 < 0.1 then
						if cch1 < 0.6 then
							set_img_out(3, x, y, 1)
						else
							set_img_out(3, x, y, 0)
						end
					else
						set_img_out(3, x, y, 0)
					end

					set_img_out(0, x, y, redChannel)
					set_img_out(1, x, y, 0)
					set_img_out(2, x, y, 0)
				end
			end
		end
	end
end
