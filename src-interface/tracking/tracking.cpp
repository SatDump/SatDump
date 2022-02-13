#ifdef BUILD_TRACKING

#include "tracking.h"
#include "common/image/image.h"
#include "resources.h"
#include "imgui/imgui_flags.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "settings.h"
#include "module.h"

TrackingUI::TrackingUI()
{
    // Load image
    image::Image<uint8_t> map_image;
    map_image.load_jpeg(resources::getResourcePath("maps/nasa.jpg").c_str());

    // Prepare texture
    map_texture = makeImageTexture();
    uint32_t *map_texture_buff = new uint32_t[map_image.width() * map_image.height()];
    uchar_to_rgba(map_image.data(), map_texture_buff, map_image.width() * map_image.height(), 3);
    updateImageTexture(map_texture, map_texture_buff, map_image.width(), map_image.height());

    // Cleanup
    delete[] map_texture_buff;

    // Start update thread
    tracking_thread = std::thread(&TrackingUI::updateThread, this);

    // Deserialize groups
    if (settings.count("tracking") > 0)
    {
        if (settings["tracking"].count("qths") > 0)
        {
            for (int i = 0; i < settings["tracking"]["qths"].size(); i++)
                qth_list.push_back(std::make_shared<GroundQTH>(settings["tracking"]["qths"][i]));
        }
        else
        {
            qth_list.push_back(std::make_shared<GroundQTH>(geodetic::geodetic_coords_t(0, 0, 0), "Default"));
        }
    }
    else
    {
        qth_list.push_back(std::make_shared<GroundQTH>(geodetic::geodetic_coords_t(0, 0, 0), "Default"));
    }
}

TrackingUI::~TrackingUI()
{
}

void TrackingUI::updateThread()
{
    while (true)
    {
        if (liveTime) // We may NOT be running live but user-set time
            current_time = time(NULL);
        else if (simulatedLiveTime)
            current_time = time(NULL) - liveSimulatedTimeOffset;

        // Calculate all positions there
        tracker.updateToTime(current_time);

        eventMutex.lock();
        upcomingevents = tracker.getObjectStatuses(qth_list[qth_selection].get());
        std::sort(upcomingevents.begin(), upcomingevents.end(), [](const ObjectStatus &lhs, const ObjectStatus &rhs)
                  { return lhs.timeUntilNextEvent < rhs.timeUntilNextEvent; });
        eventMutex.unlock();

        azElMutex.lock();
        // Update AzEl
        for (std::shared_ptr<ObjectStruct> obj : tracker.satellites)
            if (obj->isSelected)
                tracker.updateUpcomingAzElForObject(obj.get(), qth_list[qth_selection].get());
        azElMutex.unlock();

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

std::string timeToString(time_t time)
{
    std::tm *timeReadable = gmtime(&time);
    std::string timestampr = std::to_string(timeReadable->tm_year + 1900) + "/" +
                             (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "/" +
                             (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + " " +
                             (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + ":" +
                             (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + ":" +
                             (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));
    return timestampr;
}

void TrackingUI::draw(int wwidth, int wheight)
{
    //window_width = ImGui::GetWindowWidth() / 6;
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
    {
        showPanel ^= 1;
        logger->info("Show / Hide panel");
    }

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({(float)wwidth, (float)wheight});
    ImGui::Begin("Tracking UI", NULL, NOWINDOW_FLAGS | ImGuiWindowFlags_NoTitleBar);
    {
        if (showPanel)
        {
            ImGui::BeginChild("TrackingUiPanel", ImVec2(ImGui::GetWindowWidth() / 6, ImGui::GetWindowHeight() - 20));

            if (ImGui::CollapsingHeader("QTH"))
            {
                std::string qthString = "";
                for (std::shared_ptr<GroundQTH> qth : qth_list)
                    qthString += qth->getName() + '\0';

                if (ImGui::Combo("##Selected", &qth_selection, qthString.c_str()))
                {
                }

                if (ImGui::Button("Add##addqth"))
                {
                    memset(newqthname, 0, 1000);
                    showAddQth = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete##delqth"))
                {
                    if (qth_list.size() > 1)
                    {
                        qth_list.erase(qth_list.begin() + qth_selection);
                    }
                    else
                    {
                        logger->error("Can't delete all QTHs!");
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Save All##saveqth"))
                {
                    for (int i = 0; i < qth_list.size(); i++)
                        settings["tracking"]["qths"][i] = qth_list[i]->serialize();
                    saveSettings();
                }

                if (showAddQth)
                {
                    ImGui::InputText("Name##newqthname", newqthname, 1000);
                    ImGui::InputFloat("Latitude##newqthlat", &newQthLat);
                    ImGui::InputFloat("Longitude##newqthlon", &newQthLon);
                    ImGui::InputFloat("Altitude##newqthalt", &newQthAlt);

                    //ImGui::SameLine();
                    if (ImGui::Button("Ok##newqthOk"))
                    {
                        showAddQth = false;
                        std::string newName(newqthname);

                        logger->info("Adding QTH " + newName);

                        qth_list.push_back(std::make_shared<GroundQTH>(geodetic::geodetic_coords_t(newQthLat, newQthLon, newQthAlt), newName));
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel##newqthcancel"))
                        showAddQth = false;
                }
            }

            if (ImGui::CollapsingHeader("Polar Plot"))
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();

                ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                int size = ImGui::GetWindowSize().x;
                draw_list->AddCircle({cursorPos.x + (size / 2), cursorPos.y + (size / 2)}, size * 1.0 * 0.5, ImColor(0, 255, 0));
                draw_list->AddCircle({cursorPos.x + (size / 2), cursorPos.y + (size / 2)}, size * 0.666 * 0.5, ImColor(0, 255, 0));
                draw_list->AddCircle({cursorPos.x + (size / 2), cursorPos.y + (size / 2)}, size * 0.333 * 0.5, ImColor(0, 255, 0));
                draw_list->AddLine({cursorPos.x + (size / 2), cursorPos.y}, {cursorPos.x + (size / 2), cursorPos.y + size}, ImColor(0, 255, 0));
                draw_list->AddLine({cursorPos.x, cursorPos.y + (size / 2)}, {cursorPos.x + size, cursorPos.y + (size / 2)}, ImColor(0, 255, 0));

                azElMutex.lock();
                for (std::shared_ptr<ObjectStruct> obj : tracker.satellites)
                {
                    if (!obj->isEnabled)
                        continue;

                    AzEl azel = tracker.getAzElForObject(obj.get(), qth_list[qth_selection].get());
                    if (azel.el < 0)
                        continue;

                    double el = ((90 - azel.el) / (90)) * (size / 2);
                    double x = cursorPos.x + (size / 2) + sin(azel.az * DEG_TO_RAD) * el;
                    double y = cursorPos.y + (size / 2) + -cos(azel.az * DEG_TO_RAD) * el;

                    draw_list->AddCircleFilled(ImVec2(x, y), 4, ImColor(255, 0, 0));
                    draw_list->AddText(ImVec2(x, y + 4), ImColor(255, 0, 0), obj->name.c_str());

                    if (obj->isSelected)
                    {
                        std::vector<AzEl> coming = tracker.getUpcomingAzElForObject(obj.get(), qth_list[qth_selection].get());
                        if (coming.size() > 0)
                        {
                            for (int i = 0; i < coming.size() - 1; i++)
                            {
                                AzEl azel = coming[i];
                                AzEl azel1 = coming[i + 1];

                                double el = ((90 - azel.el) / (90)) * (size / 2);
                                double x = cursorPos.x + (size / 2) + sin(azel.az * DEG_TO_RAD) * el;
                                double y = cursorPos.y + (size / 2) + -cos(azel.az * DEG_TO_RAD) * el;

                                double el1 = ((90 - azel1.el) / (90)) * (size / 2);
                                double x1 = cursorPos.x + (size / 2) + sin(azel1.az * DEG_TO_RAD) * el1;
                                double y1 = cursorPos.y + (size / 2) + -cos(azel1.az * DEG_TO_RAD) * el1;

                                //if (sqrt(pow(x - x1, 2) + pow(y - y1, 2)) < 200)
                                draw_list->AddLine(ImVec2(x, y), ImVec2(x1, y1), ImColor(((SatelliteObject *)obj.get())->trace_r, ((SatelliteObject *)obj.get())->trace_g, ((SatelliteObject *)obj.get())->trace_b));
                            }
                        }
                    }
                    //std::string stringtoshow = objectToTack->getObjectName() + " El " + std::to_string(observation.elevation * RAD_TO_DEG) + " Az " + std::to_string(observation.azimuth * RAD_TO_DEG);
                    //ImGui::Text(stringtoshow.c_str());
                }
                azElMutex.unlock();

                ImGui::Dummy({float(size), float(size)});
            }

            if (ImGui::CollapsingHeader("Upcoming Events"))
            {
                if (ImGui::BeginTable("UpcomingEvents", 4, ImGuiTableFlags_Borders))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(ImColor(255, 255, 255), "Satellite");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(255, 255, 255), "Az");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextColored(ImColor(255, 255, 255), "El");
                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextColored(ImColor(255, 255, 255), "Next LOS/AOS");

                    eventMutex.lock();
                    for (ObjectStatus &status : upcomingevents)
                    {
                        ImColor color = status.isVisible ? ImColor(0, 255, 0) : ImColor(255, 0, 0);
                        if (status.obj->isSelected)
                            color = ImColor(255, 255, 0);

                        int secondsvalue = int(status.timeUntilNextEvent) % 60;
                        int minutevalue = (int(status.timeUntilNextEvent) % 3600) / 60;

                        std::string timeString = (minutevalue > 9 ? "" : "0") + std::to_string(minutevalue) + ":" + (secondsvalue > 9 ? "" : "0") + std::to_string(secondsvalue);
                        if (int(status.timeUntilNextEvent) / 3600 > 0)
                            timeString = std::to_string(int(status.timeUntilNextEvent) / 3600) + ":" + timeString;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextColored(color, status.obj->name.c_str());
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(color, "%.2f°", status.azel.az);
                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextColored(color, "%.2f°", status.azel.el);
                        ImGui::TableSetColumnIndex(3);
                        ImGui::TextColored(color, std::string(status.timeUntilNextEvent == -1 ? "Never" : timeString).c_str());
                    }
                    eventMutex.unlock();
                }
                ImGui::EndTable();
            }

            if (ImGui::CollapsingHeader("Satellites"))
            {
                tracker.drawMenu();
            }

            if (ImGui::CollapsingHeader("Time Control"))
            {
                time_t timet = current_time;
                std::tm time_struct = *gmtime(&timet);

                bool modified = false;

                modified |= ImGui::InputInt("Hours", &time_struct.tm_hour);
                modified |= ImGui::InputInt("Minutes", &time_struct.tm_min);
                modified |= ImGui::InputInt("Seconds", &time_struct.tm_sec);

                ImGui::Separator();

                time_struct.tm_year += 1900;
                modified |= ImGui::InputInt("Year", &time_struct.tm_year);
                modified |= ImGui::InputInt("Month", &time_struct.tm_mon);
                modified |= ImGui::InputInt("Day", &time_struct.tm_mday);
                time_struct.tm_year -= 1900;

                if (modified)
                    current_time = timegm(&time_struct);

                // TODO
                ImGui::Checkbox("Live", &liveTime);
                ImGui::SameLine();
                if (ImGui::Checkbox("Simulated Live", &simulatedLiveTime))
                {
                    liveSimulatedTimeOffset = time(0) - current_time;
                }
            }

            ImGui::EndChild();
        }

        ImGui::SameLine();

        ImGui::BeginChild("TrackingUiMap"); //, ImVec2(ImGui::GetWindowWidth() / 4, ImGui::GetWindowHeight()));

        ImGui::SetCursorPos({0, 0});
        ImGui::Image((void *)(intptr_t)map_texture, {ImGui::GetWindowWidth(), ImGui::GetWindowHeight() - 8});
        //ImGui::GetCur({0, 0});
        ImGui::SetCursorPos({0, 0});

        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        //draw_list->AddImage((void *)(intptr_t)map_texture, {0, 0}, {ImGui::GetWindowWidth(), ImGui::GetWindowHeight() - 8});

        tracker.satellites_mutex.lock();
        for (std::shared_ptr<ObjectStruct> obj : tracker.satellites)
        {
            if (!obj->isEnabled)
                continue;

            geodetic::geodetic_coords_t position = obj->position;

            int x, y;
            toMapCoords(position.lat, position.lon, x, y);

            draw_list->AddCircleFilled(ImVec2(x, y), 4, ImColor(255, 0, 0));
            draw_list->AddText(ImVec2(x, y + 4), ImColor(255, 0, 0), obj->name.c_str());

            ImVec2 mousevec = ImGui::GetMousePos();
            bool hovered = false;
            bool leftClicked = false;
            bool rightClicked = false;
            if (sqrt(pow(mousevec.x - x, 2) + pow(mousevec.y - y, 2)) < 20) // || obj->showMenu)
            {
                draw_list->AddCircleFilled(ImVec2(x, y), 4, ImColor(255, 255, 0));
                hovered = true;
                leftClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                rightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
            }

            tracker.drawElementsOnMap(hovered, leftClicked, rightClicked, *this, obj.get());
        }
        tracker.satellites_mutex.unlock();

        for (std::shared_ptr<GroundQTH> qth : qth_list)
        {
            geodetic::geodetic_coords_t position = qth->getPosition();

            int x, y;
            toMapCoords(position.lat, position.lon, x, y);

            draw_list->AddCircleFilled(ImVec2(x, y), 4, ImColor(0, 255, 0));
            draw_list->AddText(ImVec2(x, y + 4), ImColor(0, 255, 0), qth->getName().c_str());

            ImVec2 mousevec = ImGui::GetMousePos();
            bool hovered = false;
            if (sqrt(pow(mousevec.x - x, 2) + pow(mousevec.y - y, 2)) < 20) //|| objectToTack->showMenu)
            {
                draw_list->AddCircleFilled(ImVec2(x, y), 4, ImColor(255, 255, 0));
                hovered = true;

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    std::vector<ObjectPass> all_passes;

                    for (std::shared_ptr<ObjectStruct> obj : tracker.satellites)
                    {
                        if (obj->isEnabled)
                        {
                            std::vector<ObjectPass> passes = tracker.getUpcomingPassForObjectAtQTH(obj.get(), qth.get(), current_time + passes_delta);
                            all_passes.insert(all_passes.end(), passes.begin(), passes.end());
                        }
                    }

                    std::sort(all_passes.begin(), all_passes.end(), [](const ObjectPass &lhs, const ObjectPass &rhs)
                              { return lhs.aos_time < rhs.aos_time; });

                    //for (ObjectPass &pass : all_passes)
                    //{
                    //    std::string satname = pass.obj->name;
                    //    std::string log = "Pass of " + satname + " AOS (" + timeToString(pass.aos_time + 3600) + ")" + " LOS (" + timeToString(pass.los_time + 3600) + ") Max El " + std::to_string(round(pass.tca_azel.el)) + "deg";
                    //    logger->debug(log);
                    //}

                    qth->upcomingPasses = all_passes;
                    qth->showMenu = true;
                }
            }
        }

        // Render main Map
        ImGui::EndChild();
    }
    ImGui::End();

    tracker.drawWindows();

    for (std::shared_ptr<GroundQTH> qth : qth_list)
    {
        if (qth->showMenu)
        {
            if (ImGui::Begin(std::string("Passes for QTH " + qth->getName()).c_str(), &qth->showMenu))
            {
                if (ImGui::BeginTable("UpcomingPasses", 4, ImGuiTableFlags_Borders))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(ImColor(255, 255, 255), "Satellite");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(255, 255, 255), "AOS");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextColored(ImColor(255, 255, 255), "LOS");
                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextColored(ImColor(255, 255, 255), "Max El");

                    for (ObjectPass &pass : qth->upcomingPasses)
                    {
                        ImColor color(255, 255, 255);
                        if (pass.aos_time < current_time && pass.los_time > current_time)
                            color = ImColor(0, 255, 0);

                        std::string aosString = timeToString(pass.aos_time);
                        std::string losString = timeToString(pass.los_time);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextColored(color, pass.obj->name.c_str());
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(color, aosString.c_str());
                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextColored(color, losString.c_str());
                        ImGui::TableSetColumnIndex(3);
                        ImGui::TextColored(color, "%.2f°", pass.tca_azel.el);
                    }
                }
                ImGui::EndTable();
                ImGui::End();
            }

            if (qth->upcomingPasses.size() > 0)
            {
                if (qth->upcomingPasses[0].los_time < current_time) // If we are showing passes and the first one is over, recompute
                {
                    logger->info("Recomputing passes!");

                    std::vector<ObjectPass> all_passes;

                    for (std::shared_ptr<ObjectStruct> obj : tracker.satellites)
                    {
                        if (obj->isEnabled)
                        {
                            std::vector<ObjectPass> passes = tracker.getUpcomingPassForObjectAtQTH(obj.get(), qth.get(), current_time + passes_delta);
                            all_passes.insert(all_passes.end(), passes.begin(), passes.end());
                        }
                    }

                    std::sort(all_passes.begin(), all_passes.end(), [](const ObjectPass &lhs, const ObjectPass &rhs)
                              { return lhs.aos_time < rhs.aos_time; });

                    qth->upcomingPasses = all_passes;
                }
            }
        }
    }
}

void TrackingUI::toMapCoords(float lat, float lon, int &x, int &y)
{
    int map_height = ImGui::GetWindowHeight() - 8;
    int map_width = ImGui::GetWindowWidth();
    int imageLat = map_height - ((90.0f + lat) / 180.0f) * map_height;
    int imageLon = (lon / 360.0f) * map_width + (map_width / 2);
    if (imageLon >= map_width)
        imageLon -= map_width;
    x = ImGui::GetCursorScreenPos().x + imageLon;
    y = ImGui::GetCursorScreenPos().y + imageLat;
}

#endif