#pragma once

#include "../Action.h"
#include "UseItemAction.h"
#include "../../PlayerbotAIConfig.h"

namespace ai
{
    class DrinkAction : public UseItemAction
    {
    public:
        DrinkAction(PlayerbotAI* ai) : UseItemAction(ai, "drink") {}

        virtual bool Execute(Event event)
        {
            if (bot->isInCombat())
                return false;

            return UseItemAction::Execute(event);
        }

        virtual bool isUseful()
        {
            return UseItemAction::isUseful() && AI_VALUE2(uint8, "mana", "self target") < sPlayerbotAIConfig.mediumMana;
        }
    };

    class EatAction : public UseItemAction
    {
    public:
        EatAction(PlayerbotAI* ai) : UseItemAction(ai, "food") {}

        virtual bool Execute(Event event)
        {
            if (bot->isInCombat())
                return false;

            return UseItemAction::Execute(event);
        }

        virtual bool isUseful()
        {

            list<ObjectGuid> targets = AI_VALUE(list<ObjectGuid>, "possible targets");
            list<ObjectGuid> attackers = AI_VALUE(list<ObjectGuid>, "attackers");

            return targets.size() == attackers.size() && UseItemAction::isUseful() && AI_VALUE2(uint8, "health", "self target") < sPlayerbotAIConfig.lowHealth;
        }
    };

}
