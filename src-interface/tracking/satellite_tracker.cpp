#ifdef BUILD_TRACKING

#include "satellite_tracker.h"
#include "imgui/imgui_image.h"
#include "imgui/imgui_internal.h"
#include "tracking.h"
#include "logger.h"
#include <algorithm>
#include "settings.h"

SatelliteTracker::SatelliteTracker()
{
    // Setup objects
    //mainGroup.subgroups.push_back({"NOAA", {}, {25338, 28654, 33591}});
    //mainGroup.subgroups.push_back({"MetOp", {}, {29499, 38771, 43689}});
    //mainGroup.subgroups.push_back({"METEOR-M", {}, {35865, 40069, 44387}});
    //mainGroup.subgroups.push_back({"EOS", {}, {25994, 27424, 28376}});
    //mainGroup.subgroups.push_back({"FengYun-3", {}, {32958, 37214, 39260, 43010, 49008}});
    //mainGroup.subgroups.push_back({"JPSS", {}, {37849, 43013}});
    //mainGroup.subgroups.push_back({"Altimetry / Wind", {}, {41240, 43662, 43600}});
    //mainGroup.subgroups.push_back({"ELEKTRO / ARKTIKA", {}, {47719, 41105}});
    //mainGroup.subgroups.push_back({"GOES", {}, {29155, 35491, 36411, 41866, 43226}});

    for (std::pair<int, tle::TLE> ephem : tle::tle_map)
        satellites.push_back(std::make_shared<SatelliteObject>(ephem.first));

    // Deserialize groups
    if (settings.count("tracking") > 0)
    {
        if (settings["tracking"].count("satellites_tracker") > 0)
        {
            if (settings["tracking"]["satellites_tracker"].count("groups") > 0)
            {
                mainGroup.deserialize(settings["tracking"]["satellites_tracker"]["groups"]);
            }
        }
    }

    updateUnsorted();
}

void SatelliteTracker::updateUnsorted()
{

    std::vector<SatelliteGroup>::iterator it = std::find_if(mainGroup.subgroups.begin(), mainGroup.subgroups.end(),
                                                            [](const SatelliteGroup &m) -> bool
                                                            { return m.name == "Unsorted"; });

    if (it != mainGroup.subgroups.end())
        mainGroup.subgroups.erase(it);

    // Those that aren't in any other group go in "Unsorted"
    std::vector<int> unsorted;
    for (std::shared_ptr<SatelliteObject> satellite : satellites)
    {
        if (!mainGroup.hasSatellite(satellite->tle.norad))
        {
            //logger->info(satellite->tle.norad);
            unsorted.push_back(satellite->tle.norad);
        }
    }

    mainGroup.subgroups.push_back({"Unsorted", {}, unsorted});
}

SatelliteTracker::~SatelliteTracker()
{
}

void SatelliteTracker::updateToTime(double time)
{
    satellites_mutex.lock();
    for (std::shared_ptr<SatelliteObject> satellite : satellites)
    {
        predict_orbit(satellite->orbital_elements, &satellite->satellite_position, predict_to_julian_double(time));
        satellite->position = geodetic::geodetic_coords_t(satellite->satellite_position.latitude,
                                                          satellite->satellite_position.longitude,
                                                          satellite->satellite_position.altitude,
                                                          true)
                                  .toDegs();
        satellite->currentTime = time;

        if (satellite->drawMore)
        {
            satellite->computeFootprint();
            satellite->updateUpcoming();
        }

        satellite->ui_needs_update = true;
    }
    //current_time = time;
    satellites_mutex.unlock();
}

void SatelliteTracker::drawMenu()
{
    int node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    /*for (std::shared_ptr<SatelliteObject> satellite : satellites)
    {
        ImGui::TreeNodeEx(satellite->name.c_str(), node_flags | (satellite->isEnabled ? ImGuiTreeNodeFlags_Selected : 0));
        if (ImGui::IsItemClicked())
            satellite->isEnabled ^= 1;
    }*/
    for (SatelliteGroup &group : mainGroup.subgroups)
        displayGroup(group);

    //if (ImGui::Button("Manage##managesatellite"))
    //    showConfig = true;

    if (ImGui::Button("Add Group##addsatgroup"))
    {
        newGroupDialog = true;
        memset(newgroupname, 0, 1000);
    }

    ImGui::SameLine();
    if (ImGui::Button("Add Satellite##addsatgroup"))
    {
        newSatDialog = true;
        newNorad = 0;
    }

    ImGui::SameLine();
    if (ImGui::Button("Update TLEs##updateTLEs"))
        tle::updateTLEsMT();

    ImGui::SameLine();
    if (ImGui::Button("Save##savesatgroups"))
    {
        settings["tracking"]["satellites_tracker"]["groups"] = mainGroup.serialize();
        saveSettings();
    }

    if (newGroupDialog)
    {
        ImGui::InputText("Name##newgroupname", newgroupname, 1000);
        //ImGui::SameLine();
        if (ImGui::Button("Ok##newgroupOk"))
        {
            newGroupDialog = false;
            std::string newName(newgroupname);

            logger->info("Adding group " + newName);

            if (!mainGroup.hasGroup(newName) && newName.length() > 0)
                mainGroup.subgroups.push_back({newName, {}, {}});
            updateUnsorted();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##newgroupcancel"))
            newGroupDialog = false;
    }

    if (newSatDialog)
    {
        ImGui::InputInt("NORAD##newsatnorad", &newNorad);
        //ImGui::SameLine();
        if (ImGui::Button("Ok##newsatOk"))
        {
            newSatDialog = false;

            logger->info("Adding satellite with NORAD " + std::to_string(newNorad));

            // Add sat
            tle::fetchOrUpdateOne(newNorad);
            satellites.push_back(std::make_shared<SatelliteObject>(newNorad));

            updateUnsorted();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##newsatcancel"))
            newSatDialog = false;
    }

    //if(ImGui::Button("Add Group"))
}

void SatelliteTracker::displayGroup(SatelliteGroup &group, bool clicked, bool status)
{
    int node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    bool open = ImGui::TreeNodeEx(group.name.c_str(), node_flags | (group.isEnabled ? ImGuiTreeNodeFlags_Selected : 0));

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("_DRAGDROPSAT"))
        {
            DragDropSat dragdrop = *(const DragDropSat *)payload->Data;
            group.satellites.push_back(dragdrop.norad);
            dragdrop.group->satellites.erase(std::find(dragdrop.group->satellites.begin(), dragdrop.group->satellites.end(), dragdrop.norad));
            logger->info(group.name + " add " + std::to_string(dragdrop.norad));
            updateUnsorted();
            //logger->info(dragdrop.norad);
        }
        else if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("_DRAGDROPSATGROUP"))
        {
            DragDropGroup dragdrop = *(const DragDropGroup *)payload->Data;
            std::string currentgrp = group.name;
            SatelliteGroup gr = *dragdrop.group;
            mainGroup.removeGroup(gr.name);

            std::find_if(mainGroup.subgroups.begin(), mainGroup.subgroups.end(),
                         [&currentgrp](const SatelliteGroup &m) -> bool
                         { return m.name == currentgrp; })
                ->subgroups.push_back(gr);

            logger->info(currentgrp + " add " + gr.name);
            //dragdrop.group->satellites.erase(std::find(dragdrop.group->satellites.begin(), dragdrop.group->satellites.end(), dragdrop.norad));
            updateUnsorted();
            //logger->info(dragdrop.norad);
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        DragDropGroup dragdrop = {&group};
        ImGui::SetDragDropPayload("_DRAGDROPSATGROUP", &dragdrop, sizeof(dragdrop));
        ImGui::Text(group.name.c_str());
        ImGui::EndDragDropSource();
    }

    ImGui::TreePush();

    bool wasClicked = false;
    ImGuiContext &g = *GImGui;
    if (ImGui::IsItemClicked() && g.ActiveIdClickOffset.x > g.FontSize) // Avoid click on the arrow triggering this
    {
        group.isEnabled ^= 1;
        wasClicked = true;
    }

    if (clicked)
    {
        group.isEnabled = status;
        wasClicked = true;
    }

    std::vector<int> toRemove;
    for (int &norad : group.satellites)
    {
        const std::shared_ptr<SatelliteObject> satellite = *std::find_if(satellites.begin(), satellites.end(),
                                                                         [&norad](const std::shared_ptr<SatelliteObject> &m) -> bool
                                                                         { return m->tle.norad == norad; })
                                                                .base();

        if (open)
        {
            int node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_OpenOnDoubleClick;

            ImGui::TreeNodeEx(satellite->name.c_str(), node_flags | (satellite->isEnabled ? ImGuiTreeNodeFlags_Selected : 0));
            if (ImGui::IsItemClicked())
                satellite->isEnabled ^= 1;

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::Button("Remove from group"))
                {
                    toRemove.push_back(norad);
                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::Button("Cancel"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                DragDropSat dragdrop = {norad, &group};
                ImGui::SetDragDropPayload("_DRAGDROPSAT", &dragdrop, sizeof(dragdrop));
                ImGui::Text(satellite->name.c_str());
                ImGui::EndDragDropSource();
            }
        }

        if (wasClicked)
            satellite->isEnabled = group.isEnabled;
    }

    if (toRemove.size() > 0)
    {
        for (int norad : toRemove)
            group.satellites.erase(std::find(group.satellites.begin(), group.satellites.end(), norad));
        updateUnsorted();
    }

    if (open)
    {
        int en = group.isEnabled;
        for (SatelliteGroup &group : group.subgroups)
            displayGroup(group, wasClicked, en);
    }

    ImGui::TreePop();
}

void SatelliteTracker::drawWindows()
{
    for (std::shared_ptr<SatelliteObject> satellite : satellites)
    {
        if (satellite->showMenu)
        {
            ImGui::Begin(std::string("Satellite " + satellite->name).c_str(), &satellite->showMenu);
            {
                ImGui::Text(std::string("NORAD : " + std::to_string(satellite->tle.norad)).c_str());
                ImGui::Text(std::string("Latitude : " + std::to_string(satellite->position.lat) + "°").c_str());
                ImGui::Text(std::string("Lontitude : " + std::to_string(satellite->position.lon) + "°").c_str());
                ImGui::Text(std::string("Altitude : " + std::to_string(satellite->position.alt) + "km").c_str());
                ImGui::Text(std::string("Footprint : " + std::to_string(satellite->satellite_position.footprint) + "km").c_str());
                ImGui::Text(std::string("Eclipsed : " + std::string(satellite->satellite_position.eclipsed ? "Yes" : "No")).c_str());
                ImGui::Text(std::string("Revolution : " + std::to_string(satellite->satellite_position.revolutions)).c_str());
            }
            ImGui::End();
        }
    }

    if (showConfig)
    {
        ImGui::Begin(std::string("Satellites Configuration").c_str(), &showConfig);
        {
            for (SatelliteGroup &group : mainGroup.subgroups)
                displayGroup(group);
        }
        ImGui::End();
    }
}

void SatelliteTracker::drawElementsOnMap(bool isHovered, bool leftClicked, bool rightClicked, TrackingUI &ui, ObjectStruct *object)
{
    SatelliteObject *satellite = (SatelliteObject *)object;

    int x, y, x1, y1;
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    if (leftClicked)
        satellite->drawMore ^= 1;
    satellite->isSelected = satellite->drawMore;

    if (rightClicked)
        satellite->showMenu ^= 1;

    if (isHovered || satellite->drawMore)
    {
        if (isHovered && !satellite->drawMore && satellite->ui_needs_update)
        {
            satellite->updateUpcoming();
            satellite->computeFootprint();
            satellite->ui_needs_update = false;
        }

        // Render upcoming orbit
        {
            satellite->upcomingMutex.lock();
            if (satellite->upcomingOrbit.size() > 0)
            {
                for (int i = 0; i < satellite->upcomingOrbit.size() - 1; i++)
                {
                    ui.toMapCoords(satellite->upcomingOrbit[i].lat, satellite->upcomingOrbit[i].lon, x, y);
                    ui.toMapCoords(satellite->upcomingOrbit[i + 1].lat, satellite->upcomingOrbit[i + 1].lon, x1, y1);
                    if (sqrt(pow(x - x1, 2) + pow(y - y1, 2)) < 200)
                        draw_list->AddLine(ImVec2(x, y), ImVec2(x1, y1), ImColor(satellite->trace_r, satellite->trace_g, satellite->trace_b));
                }
            }
            satellite->upcomingMutex.unlock();
        }

        // Draw footprint
        {
            satellite->footprintMutex.lock();
            for (int i = 1; i < 360; i++)
            {
                ui.toMapCoords(satellite->footprint_points[i - 1].lat, satellite->footprint_points[i - 1].lon, x, y);
                ui.toMapCoords(satellite->footprint_points[i].lat, satellite->footprint_points[i].lon, x1, y1);

                if (sqrt(pow(x - x1, 2) + pow(y - y1, 2)) < 200)
                    draw_list->AddLine(ImVec2(x, y), ImVec2(x1, y1), ImColor(255, 0, 255));
            }

            ui.toMapCoords(satellite->footprint_points[0].lat, satellite->footprint_points[0].lon, x, y);
            ui.toMapCoords(satellite->footprint_points[359].lat, satellite->footprint_points[359].lon, x1, y1);
            if (sqrt(pow(x - x1, 2) + pow(y - y1, 2)) < 200)
                draw_list->AddLine(ImVec2(x, y), ImVec2(x1, y1), ImColor(255, 0, 255));
            satellite->footprintMutex.unlock();
        }
    }
}

AzEl SatelliteTracker::getAzElForObject(ObjectStruct *object, GroundQTH *qth)
{
    SatelliteObject *satellite = (SatelliteObject *)object;
    predict_observation observation;
    predict_observe_orbit(qth->get_predict(), &satellite->satellite_position, &observation);
    return {float(observation.azimuth * RAD_TO_DEG), float(observation.elevation * RAD_TO_DEG)};
}

std::vector<ObjectStatus> SatelliteTracker::getObjectStatuses(GroundQTH *qth)
{
    std::vector<ObjectStatus> objectStatuses;
    for (std::shared_ptr<SatelliteObject> satellite : satellites)
    {
        if (!satellite->isEnabled)
            continue;

        bool is_geo = predict_is_geosynchronous(satellite->orbital_elements);
        AzEl azEl = getAzElForObject(satellite.get(), qth);

        if (is_geo && azEl.el > 0)
        {
            objectStatuses.push_back({satellite.get(), -1, azEl.el > 0, azEl});
            continue;
        }

        predict_observation next_aos = predict_next_aos(qth->get_predict(), satellite->orbital_elements, predict_to_julian(satellite->currentTime));
        predict_observation next_los = predict_next_los(qth->get_predict(), satellite->orbital_elements, predict_to_julian(satellite->currentTime));

        double time_until_aos = predict_from_julian(next_aos.time) - satellite->currentTime;
        double time_until_los = predict_from_julian(next_los.time) - satellite->currentTime;

        //logger->info(time_until_aos);
        //logger->info(time_until_los);

        if (time_until_aos < time_until_los)
        {
            objectStatuses.push_back({satellite.get(), time_until_aos, azEl.el > 0, azEl});
            continue;
        }
        else if (time_until_los < time_until_aos)
        {
            objectStatuses.push_back({satellite.get(), time_until_los, azEl.el > 0, azEl});
            continue;
        }
    }

    // logger->info(objectStatuses.size());

    return objectStatuses;
}

void SatelliteTracker::updateUpcomingAzElForObject(ObjectStruct *object, GroundQTH *qth)
{
    SatelliteObject *satellite = (SatelliteObject *)object;
    satellite->updateUpcomingAzEl(qth);
}

std::vector<AzEl> SatelliteTracker::getUpcomingAzElForObject(ObjectStruct *object, GroundQTH *qth)
{
    SatelliteObject *satellite = (SatelliteObject *)object;
    std::vector<AzEl> upcoming;

    if (satellite->upcomingAzEl.count(qth->getName()) > 0)
        upcoming = satellite->upcomingAzEl[qth->getName()];

    return upcoming;
}

std::vector<ObjectPass> SatelliteTracker::getUpcomingPassForObjectAtQTH(ObjectStruct *object, GroundQTH *qth, double timeEnd)
{
    SatelliteObject *satellite = (SatelliteObject *)object;
    std::vector<ObjectPass> passes;

    if (predict_is_geosynchronous(satellite->orbital_elements)) // There are no passes for GEOs
        return passes;

    int currentPassTime = satellite->currentTime;

    predict_observation next_tca;
    predict_position predict_orb;

    predict_orbit(satellite->orbital_elements, &predict_orb, predict_to_julian(currentPassTime));
    predict_observe_orbit(qth->get_predict(), &predict_orb, &next_tca);

    if (next_tca.elevation > 0)
    {
        while (next_tca.elevation > 0)
        {
            predict_orbit(satellite->orbital_elements, &predict_orb, predict_to_julian(currentPassTime));
            predict_observe_orbit(qth->get_predict(), &predict_orb, &next_tca);
            currentPassTime--;
        }
    }

    while (currentPassTime < timeEnd)
    {
        ObjectPass currentPass;
        currentPass.obj = object;

        // Get AOS
        predict_observation next_aos = predict_next_aos(qth->get_predict(), satellite->orbital_elements, predict_to_julian(currentPassTime));
        currentPass.aos_azel = {float(next_aos.azimuth * RAD_TO_DEG), float(next_aos.elevation * RAD_TO_DEG)};
        currentPass.aos_time = predict_from_julian(next_aos.time);

        currentPassTime = currentPass.aos_time;

        // Get LOS
        predict_observation next_los = predict_next_los(qth->get_predict(), satellite->orbital_elements, predict_to_julian(currentPassTime));
        currentPass.los_azel = {float(next_los.azimuth * RAD_TO_DEG), float(next_los.elevation * RAD_TO_DEG)};
        currentPass.los_time = predict_from_julian(next_los.time);

        // Compute TCA
        float maxEl = 0;
        for (time_t passtime = currentPass.aos_time; passtime < currentPass.los_time; passtime += 1)
        {
            predict_orbit(satellite->orbital_elements, &predict_orb, predict_to_julian(passtime));
            predict_observe_orbit(qth->get_predict(), &predict_orb, &next_tca);
            if (maxEl < next_tca.elevation * RAD_TO_DEG)
            {
                maxEl = next_tca.elevation * RAD_TO_DEG;
                currentPass.tca_time = passtime;
                currentPass.tca_azel = {float(next_tca.azimuth * RAD_TO_DEG), float(next_tca.elevation * RAD_TO_DEG)};
            }
        }

        currentPassTime = currentPass.los_time + 1;

        if (currentPass.tca_azel.el > 0 && currentPass.aos_time < currentPass.los_time)
            passes.push_back(currentPass);
    }

    return passes;
}

#endif