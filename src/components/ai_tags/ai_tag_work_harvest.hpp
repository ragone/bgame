#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>
#include "../position.hpp"
#include "../../bengine/ecs.hpp"

struct ai_tag_work_harvest {

    enum harvest_steps { FIND_HARVEST, DO_HARVEST };

    ai_tag_work_harvest() {}
    harvest_steps step = FIND_HARVEST;

    template<class Archive>
    void serialize(Archive & archive)
    {
        // Nothing to save
        archive( step );
    }
};

CEREAL_REGISTER_TYPE(bengine::impl::component_store_t<bengine::impl::component_t<ai_tag_work_harvest>>)
