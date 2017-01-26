#include "settler_ai.hpp"

void settler_ai_t::to_xml(xml_node * c) {
    component_to_xml(c,
        std::make_pair("shift_id", shift_id),
        std::make_pair("job_type_major", job_type_major),
        std::make_pair("job_status", job_status),
        std::make_pair("job_type_minor", job_type_minor),
        std::make_pair("target_x", target_x),
        std::make_pair("target_y", target_y),
        std::make_pair("target_z", target_z),
        std::make_pair("target_id", target_id),
        std::make_pair("has_building_target", has_building_target),
        std::make_pair("building_target", building_target),
        std::make_pair("has_reaction_target", has_reaction_target),
        std::make_pair("reaction_target", reaction_target),
        std::make_pair("current_tool", current_tool)
    );
    for (int i=0; i<NUMBER_OF_JOB_CATEGORIES; ++i) {
        c->add_value(std::string("permitted_work_") + std::to_string(i), std::to_string(permitted_work[i]));
    }
}

void settler_ai_t::from_xml(xml_node * c) {
    shift_id = c->val<int>("shift_id");
    job_type_major = (job_major_t)c->val<int>("job_type_major");
    job_status = c->val<std::string>("job_status");
    job_type_minor = (job_minor_t)c->val<int>("job_type_minor");
    target_x = c->val<int>("target_x");
    target_y = c->val<int>("target_y");
    target_z = c->val<int>("target_z");    
    current_tool = c->val<std::size_t>("current_tool");
    has_building_target = c->val<bool>("has_building_target");
    has_reaction_target = c->val<bool>("has_reaction_target");

    if (has_building_target) {
        building_designation_t b;
        b.from_xml(c->find("building_target"));
        building_target = b;
    }

    if (has_reaction_target) {
        reaction_task_t r;
        r.from_xml(c->find("reaction_target"));
        reaction_target = r;
    }

    for (int i=0; i<NUMBER_OF_JOB_CATEGORIES; ++i) {
        const std::string key = std::string("permitted_work_") + std::to_string(i);
        permitted_work[i] = c->val<bool>(key);
    }
}