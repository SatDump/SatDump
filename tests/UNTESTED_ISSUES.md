# Crash and undefined-behaviour problems without tests

These problems were found by reading `src-core` while the unit tests were being written. None of them has a test, because exercising them would crash or hang the test executable or rely on undefined behaviour.

- **Status:** reported from code reading, mostly not independently verified. Two were observed crashing (marked "seen"). Treat each entry as a lead to confirm, not a confirmed defect.
- **Paths** are relative to `src-core/`. Line numbers are as of commit `9fee0793c` and will drift.
- **Not listed here:** bugs that do have a test. Those are tagged `[!shouldfail]`; list them with `./satdump_tests --list-tests "[!shouldfail]"`.

When one of these is fixed, add a regular test for it and remove the entry.

## common

| Location | Problem | Effect |
|---|---|---|
| `common/ziq.cpp` (writer and reader constructors, destructors) | In uncompressed mode `max_buffer_size` is used uninitialised, and the destructors call `ZSTD_freeCCtx` / `ZSTD_freeDCtx` on a context pointer that was never set. | Uninitialised read; free of a garbage pointer. |
| `common/ziq.cpp` `isValidZIQ`, `common/ziq2.cpp` `ziq2_is_valid_ziq2` | The signature buffer is compared without being filled when the file is missing or shorter than 4 bytes. | Uninitialised read. |
| `common/mpeg_ts/fazzt_processor.cpp` | For body packets of 8 to 15 bytes, `fazzt_frame.size() - 16` goes negative and is used as a `memcpy` length; `frame[8..9]` is also read out of bounds. | Huge out-of-bounds copy. |
| `common/audio/audio_sink.cpp` `resample_s16` | Reads `input[c + channels]`, one frame past the end of the input. With more than one channel it writes `outputSize * channels` samples although the returned size is a total sample count. | Out-of-bounds read and write. |
| `common/map/map_drawer.cpp:148` | The check is `cc.first == -1 \|\| cc.first == -1` (the second should be `cc.second`), so a GeoJSON point projected to y = -1 reaches `draw_pixel`, which has no bounds check. | Out-of-bounds write. |
| `common/tracking/tracking.cpp` `SatelliteTracker::get_observed_position` | On an ephemeris-based tracker the orbit object is null and is dereferenced. | Null dereference. |
| `common/tracking/interpolator.h:63-77` | `LinearInterpolator` with exactly two points indexes past the end of its table. | Out-of-bounds read. |
| `common/tracking/rotator/rotcl_handler.cpp:76` | The address is copied with `memcpy` into a 100-byte buffer with no length check. | Buffer overflow for long addresses. |
| `common/codings/deframing/hdlc_def.cpp:26` | The length check is `pos_in_pkt > d_len_max` and should be `>=`. A frame or noise run reaching `d_len_max` bytes writes one byte past `wip_pkt`. This includes a valid frame of exactly the maximum size. | Heap overflow by one byte. |
| `common/codings/deframing/hdlc_def.cpp` | With a minimum length below 2 the deframer reads `wip_pkt[-1]`. | Out-of-bounds read. |
| `common/codings/soft_reader.h:15-50` | In `hard_symbols` mode `tmp_hard_buff` is never filled before its first use, so the first symbols returned come from uninitialised memory. | Uninitialised read; wrong data at the start of every stream. |
| `common/ccsds/ccsds_time.cpp:10` | `parseCCSDSTime` multiplies `(offset + days) * 86400` in `int`, which overflows for dates after 2038. | Signed overflow. |
| `common/codings/differential/generic.cpp` | If the first `work` call has fewer than 2 samples it returns -1 and then erases an invalid iterator range. | Undefined behaviour on short input. |
| `common/codings/crc/crc_generic.cpp` | The non-reflected path with a width under 8 bits shifts by a negative amount. | Undefined shift. |
| `common/codings/manchester.cpp` | `manchesterDecoder` reads past the input when the length is odd. | Out-of-bounds read. |
| `common/dsp/filter/deemphasis.cpp:39-73` | The block is unusable: `d_nsamples` is 0 so its loop never advances, and it indexes empty vectors. | Hang or crash as soon as it runs. |
| `common/dsp/resamp/power_decim.cpp:16` | The check is `id > plans_len` and should be `>=`. A decimation of 16384 reads `plans[13]`. | Out-of-bounds read. |
| `common/dsp/clock_recovery/clock_recovery_mm.cpp`, `clock_recovery_gardner.cpp` | The history buffer holds `STREAM_BUFFER_SIZE` samples, but input is copied in at an offset of `ntaps - 1`. | Buffer overflow for input chunks close to `STREAM_BUFFER_SIZE`. |
| `common/dsp/clock_recovery/clock_recovery_mm.h:30`, `clock_recovery_gardner.h:29` | `last_sample` is never initialised, and the float versions use it in the first phase error (`clock_recovery_mm.cpp:82`, `clock_recovery_gardner.cpp:78`). | Uninitialised read (seen for MM): leftover bytes that read as a NaN make omega and mu NaN, `inc` stays at 0 and every input buffer gives `STREAM_BUFFER_SIZE` outputs. The tests build these blocks over zeroed memory (`helpers/prefilled.h`) so that their results are repeatable. |
| `common/dsp/path/splitter_vfo.cpp` `reset_vfo` | Replaces the stream the splitter writes to while the `FreqShiftBlock` keeps reading the old one. | The splitter blocks forever once that VFO is re-enabled. |
| `common/dsp/buffer.h` (`RingBuffer` `stopReader` / `stopWriter`) | The stop flag is set without holding the mutex. | A wakeup can be lost, leaving a blocked thread hung. |
| `common/codings/reedsolomon/reedsolomon.cpp:147`, `:153` | With `fill_bytes = -1` (the default in some pipeline modules) `deinterleave` and `interleave` loop 256 times. | Writes `buff[255]` out of bounds and one byte past each interleaved codeword. |
| `common/codings/viterbi/viterbi_punc.cpp:80`, `:158` | `d_bers[2][12][2]` is indexed with a shift of up to 15, and with the phase in the last dimension. | Out-of-bounds read in `ber()` when `check_iq_swap` is true. |
| `common/codings/viterbi/viterbi_punc.cpp:75`, `common/codings/viterbi/cc_decoder.cpp:303-312` | The lock search calls the deprecated `CCDecoder::work(in, out, size)`, which ignores `size` and decodes a full 2054-bit frame (4108 symbols) of `ber_depunc_buffer`, while `depunc_static` only wrote 2340 to 3072 of them. The rest of that member array is never written. | Uninitialised read (seen): the lock picks a wrong phase or shift depending on leftover memory, e.g. a clean stream decoded inverted. The tests build the decoder over memory filled with erasures (`helpers/prefilled.h`) so that their results are repeatable. |

## image

| Location | Problem | Effect |
|---|---|---|
| `image/bayer/bayer.cpp:2610` | In `dc1394_bayer_AHD_uint16`, `(int)y < height` is tested on an unsigned `y` that has wrapped to `UINT_MAX` at the border. The 8-bit version is correct. | Far out-of-bounds read. Segfault seen. |
| `image/histogram_utils.cpp:75` | `find_target_value_hist` calls itself with `val + 1` until `INT_MAX` when the value is absent. | Stack overflow or hang. |
| `image/io.cpp:17` | `load_img` logs that the file is missing but carries on and reads an uninitialised `signature[10]`. Files shorter than 10 bytes have the same problem. | Uninitialised read. |
| `image/io/qoiio.cpp:86` | For a corrupt file `qoi_read` returns NULL; `img.init` then uses an uninitialised `desc` and the NULL buffer is read. | Null dereference. |
| `image/io/pngio.cpp:240` | The in-memory `load_png` has no gray+alpha case, so it uses 0 channels and allocates a 0-byte row that `png_read_row` then fills. | Heap overflow. |
| `image/jpeg_utils.cpp:56` | `decompress_jpeg` allocates only `w * h` bytes. | Heap overflow on colour JPEGs. |
| `image/jpeg_utils.cpp` | `decompress_jpeg(..., ignore_errors = true)` on corrupt data may keep decoding with no exit path. | Possible hang. |
| `image/simple_mean_filter.cpp:44` | EDGE mode does not clamp, so negative results are converted from float to unsigned. A radius of 0 divides by zero. | Undefined conversion; division by zero. |
| `image/processing.cpp:57` | `white_balance` on a constant channel divides by zero and casts inf/NaN to `long`. | Undefined conversion. |
| `image/image_utils.cpp:72-88` | `blend_images` computes 0/0 for pixels that are 0 in every layer. | NaN converted to an integer. |
| `image/image_draw.cpp` `draw_aa_line` | Steep lines read with transposed coordinates. | Out-of-bounds read on non-square images. |
| `image/text.cpp` | Pixels are mis-addressed when x is negative. | Out-of-bounds write. |
| `image/image.cpp:30` | Self-assignment (`img = img`) frees the buffer in `init` before copying from it. | Use after free. |
| `image/io/` (QOI, TIFF, PNG writers) | Saving 2-channel QOI or TIFF, or PNG at depths below 8 bits, goes through uninitialised or unsupported paths. | Undefined output or behaviour. |

## dsp (new framework)

| Location | Problem | Effect |
|---|---|---|
| `dsp/utils/reshape_buffer.cpp:23-25` | `start()` launches `work2` before `Block::start()` has run `init()`, so `work2` reads an uninitialised `RingBuffer`. Two threads also write to the same single-producer stream, and the terminator can overtake data. | Segfault seen; data race. |
| `dsp/utils/delay_one_imag.h:15`, `delay_one_imag.cpp:20` | `lastSamp` is uninitialised, and an empty buffer reads `input[-1]`. | Uninitialised and out-of-bounds read. |
| `dsp/utils/hilbert.cpp:40`, `:48` | The buffer overflows by `ntaps` when the input is at least `buffer_size - ntaps` samples; for inputs larger than `buffer_size` the last chunk also reads past the input. | Buffer overflow. |
| `dsp/utils/hilbert.h:38` | `taps` is freed without being reset, so a re-init with an even tap count frees it twice. Doing this while running throws inside the block thread. | Double free; process termination. |
| `dsp/device/options_displayer.h:95-104`, `:182` | `add_options` (missing `"type"`) and `get_values` (unknown type) throw while `opts_mtx` is still locked. | The next call deadlocks. |
| `dsp/flowgraph/flowgraph.cpp:217-218` | `ic` is incremented twice, so the second input's `node_io` type is wrong or uninitialised. | Uninitialised read. |
| `dsp/path/selector.cpp` | `SelectorBlock` stops at the first terminator on any input, including inactive ones. It also busy-spins on `try_dequeue` while holding its mutex. | Drops the active branch's tail; can deadlock inside `AudioDemodHierBlock`. |
| `dsp/pll/pll_carrier_tracking.h:30-36` | `init()` reads `d_loop_bw`, which is never assigned; `d_max_freq`, `d_min_freq`, `d_phase` and `d_freq` are never initialised either. | Running the block is undefined behaviour. |
| `dsp/ddc/fft_ddc.cpp:151` | VFOs created through `set_cfg("noutputs")` get decimation 0, and `set_vfo_dec(id, 0)` is not clamped, so the code evaluates `% 0`. | SIGFPE as soon as such a VFO has a frequency or bandwidth. |
| `dsp/fft/fft_pan.cpp:12` | `output_fft_buff` is averaged into but never initialised. | Uninitialised read. |
| `dsp/pll/fll_band_edge.h:37` | The FIR history buffer is not zeroed. | Uninitialised read. |
| `dsp/fft/fft_helper.h:27` | The destructor calls `fftwf_free(plan)` instead of `fftwf_destroy_plan`. The class also shares its name with the non-template `FFTHelper` in `dsp/ddc/fft_helper.h`. | Invalid free; one-definition-rule hazard. |

## projection and products

| Location | Problem | Effect |
|---|---|---|
| `projection/projection.h`, `projection.cpp` | `Projection` frees `std_proj.proj_dat` in its destructor but has no copy constructor or copy assignment. Existing callers are only safe because they call `init()` right after assigning. | Dangling pointer and double free on copy. |
| `projection/raytrace/common/timestamp_line_gcps.cpp:133-140` | An x past the last GCP of a line leaves `start_pos == size` and reads `l.g[size]`. | Heap out-of-bounds read. |
| `projection/warp/smart_warp.cpp:372-378` | The compositing loop always touches channels 0 to 3. | Out-of-bounds access with `output_rgba = false` and a 1- or 3-channel input. |
| `projection/raytrace/common/normal_per_ifov.cpp` | `get_position` does not check for negative y or out-of-range x. | Out-of-bounds read. |
| `products/image/product_expression.cpp` | `lutProcess` and `equpProcess` check `ch > channels()` instead of `>=`. | Out-of-bounds read. |

## pipeline and handlers

| Location | Problem | Effect |
|---|---|---|
| `pipeline/live_pipeline.cpp:174` | In client mode `stop()` dereferences the first module's input buffer, which `start_client` never sets for `network_client`. | Null dereference. |
| `handlers/handler.h:57` | `delSubHandler` on a handler that is not a child, or twice on the same one, calls `erase(end())`. | Undefined behaviour. |
| `pipeline/modules/xrit/module_s2udp_xrit_cadu_extractor.cpp:69` | `payload[40..43]` is read before the `payload.size() >= 1024` check. | Out-of-bounds read on short payloads. |
| `handlers/image/image_handler.cpp:99` | `~ImageHandler` explicitly calls `ProcessingHandler::~ProcessingHandler()`, so the base destructor runs twice. | Undefined behaviour; harmless in practice so far. |
| `pipeline/pipeline.h` | `PipelinePreset::samplerate` is uninitialised when there is no samplerate parameter. | Uninitialised read. |
| `pipeline/pipeline.cpp` `loadPipelines` | Calls `exit(1)` when the pipelines directory is missing. | Terminates the host process. |

## utils, core, db and logger

| Location | Problem | Effect |
|---|---|---|
| `core/cli/cli.cpp:43-44` | The `-v,--verbose` flag is bound to `verbose`, a local variable of the `CommandHandler` constructor. `parse()` runs after the constructor has returned, so passing `-v` makes CLI11 write through a dangling reference. | Stack corruption ("stack smashing detected" seen). |
| `core/plugin.cpp:153` | The retry loop calls `pl->getID()` without a null check. If a plugin fails `dlopen` first, then loads on retry with a missing or mismatched ABI, `loadPlugin` returns null. | Null dereference. |
| `utils/task_queue.cpp:17-39` | `thread_exited = true` is set after the queue lock is released. A `push()` in that window sees the worker as still running. | The task is stranded and never runs. |
| `utils/task_scheduler.cpp` | `running` and `needs_update` are plain bools, and the destructor notifies without the mutex. | Lost wakeup; the destructor can block until the next due time. |
| `utils/task_scheduler.cpp` | With no tasks, `numeric_limits<time_t>::max()` is passed to `wait_for`. | Overflow in the chrono conversion. |
| `utils/task_scheduler.cpp` | Due events fire while `task_mtx` is held. | A handler that calls `add_task` or `del_task` deadlocks. |
| `utils/mqtt_client.h` | `run_refresh` is a non-atomic bool shared with the refresher thread. | Data race; the DISCONNECT queued in the destructor may never be sent. |
| `core/params.cpp:72-81` | For `options`, `d_option` stays uninitialised when the value is not one of the options. For `labeled_options` with an empty list and no value, `d_labeled_opts[0]` is read. | Out-of-bounds read in `getValue()`. |
| `utils/file/folder_file_iterators.h:31` | Takes `&v[0]` on an empty vector for zero-length files. | Undefined behaviour. |
| `utils/color.cpp:22` | A negative hue, or a value above 1, converts an out-of-range float to `uint8_t`. | Undefined conversion. |
| `db/kepler/kepler_utils.cpp:34-68` | For values with no short decimal form (such as 1/3) `strLE` grows its multiplier until `(int)(val * 10^mult)` overflows. | Undefined conversion. |
| `libs/predict` `predict_parse_tle` | Reads `line[51]` and `line[60]` past the end of short lines, and never returns NULL for bad input, so `tleToKepler` cannot fail. | Out-of-bounds read. |
| `db/iers/iers_handler.cpp` | `IERSInfo::time` is never set, and `leap_seconds` is left uninitialised when there is no leap-second row. | Uninitialised read. |
| `logger.cpp` | Logging at `LOG_OFF` indexes past the colour and tag tables. | Out-of-bounds read. |

## Related problems that are not crashes

These were reported alongside the entries above. They do not crash, but they also have no test.

**Resource leaks**

- `common/dsp/filter/fft_filter.cpp`: the `FFTFilterBlock` destructor never frees its FFTW plans or buffers.
- `dsp/filter/fft.cpp`: `init()` leaks buffers and plans on every re-init.
- `dsp/block_simple_multi.h:53-54`: buffers already dequeued from earlier inputs are not freed when a later input terminates.
- `common/codings/turbo/ccsds_turbo.cpp:124-131`: `encode()` leaks three buffers per call.
- `products/image/product_expression.cpp`: the `tkts` array is never deleted.
- `projection/projection.cpp`: every `init()` leaks the previous allocation.
- `image/meta.cpp`: `set_metadata` leaks the previous JSON.
- `image/io/tiffio.cpp`: `load_tiff` does not close the file when the depth is unsupported.
- `utils/mqtt_client.h`: the constructor leaks the socket and buffers when it throws after the socket is opened.
- `db/db_handler.cpp`: the SQLite handle is not closed when opening fails.
- `logger.cpp`: `logf` uses `va_copy` without `va_end`.

**Wrong results**

- `common/mpeg_ts/fazzt_processor.cpp`: `if (frame_cnt++ % 1000)` runs the expiry check on 999 of every 1000 frames, which looks inverted.
- `common/tracking/scheduler/passes.cpp:12`: passes `qth_alt * DEG_TO_RAD` as the observer altitude, which the library expects in metres.
- `projection/warp/smart_warp.cpp:256`: middle segments are generated as `[cutPositions[i], y_end]` instead of ending at the next cut. Only matters with three or more cuts.
- `dsp/channel_model/model/attenuation/atmospheric_gases_itu.cpp:272`: integer division inside `exp`; the temperature profile for 71 to 84.852 km uses the wrong formula.
- `dsp/filter/fft.cpp:122`: `FFTFilterBlock<float>` does nothing.
- `db/kepler/kepler_handler.cpp`: `putKepler` does not escape the designator in its SQL.

**Other**

- `dsp/channel_model`: `ChannelModelLEO::get_channel_model_settings` is declared but never defined.
- `projection/reprojector.cpp:104`: `reproject()` only adds the source `width` / `height` to the projection config at `:225`, in the raytraced branch. Before that, assigning the config to a `Projection` (`:97`) fails in `Projection::init` (`projection.cpp:28`, "Image width must be present!"), and the error is caught, but `src_proj.init(true, false)` is then called again outside the `try` and throws out of `reproject()`. Only `reprojectImage()` works, because it adds the width and height first.
- `dsp/resampling`: `RationalResamplerBlock::needs_reinit` is set but never read, so changes only apply on `start()`.
- `core/opencl`: `platform_id` defaults to 0, so `useCL()` returns true when OpenCL was never initialised.
- `common/tracking/interpolator.h`, `common/dsp/io/wav_writer.h`, `common/codings/ldpc/matrix/general_utils.h`: missing standard includes, so the headers do not compile on their own.
