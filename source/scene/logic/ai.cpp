﻿#include "ai.h"
#include "scene.h"
#include "sceneMgr.h"

AI::AI()
{

}
AI::~AI()
{

}
void AI::init(std::weak_ptr<Scene> scene)
{
    _scene = scene;
}


void AI::update()
{
    auto scene = _scene.lock();
    if (!scene)
    {
        return;
    }


    rebirthCheck();

    if (scene->getSceneType() == SCENE_MELEE)
    {
        if (_march.empty())
        {
            _marchOrg =  EPosition(34, 170);
            double dist = 20;
            for (size_t i = 0; i < 20; i++)
            {
                EPosition sp = toEPosition(getFarPoint(_marchOrg.x, _marchOrg.y, i*(PI*2/19.0)*1.0, dist));
                auto entity = scene->makeEntity(rand() % 20 + 1,
                    InvalidAvatarID,
                    "march",
                    DictArrayKey(),
                    InvalidGroupID);
                entity->_props.hp = 3000 + (rand() % 100) * 20;
                entity->_props.moveSpeed = 8.0;
                entity->_props.attackQuick = 0.5;
                entity->_props.attack = 80;
                entity->_state.maxHP = entity->_props.hp;
                entity->_state.curHP = entity->_state.maxHP;
                entity->_state.camp = ENTITY_CAMP_BLUE + 100;
                
                entity->_skillSys.dictEquippedSkills[2] = 0;
                entity->_skillSys.readySkillID = 2;
				

                entity->_move.position = sp;
                entity->_control.spawnpoint = sp;
                entity->_control.collision = 1.0;
                scene->addEntity(entity);
                _march.push_back(entity);
            }
        }
        double now = getFloatNowTime();
        if (now - _lastMarch > 10 && !_march.empty() )
        {
            _lastMarch = now;

            bool allIdle = true;
            for (auto e : _march)
            {
                if (e->_move.action != MOVE_ACTION_IDLE)
                {
                    allIdle = false;
                    break;
                }
            }
            if (allIdle)
            {
                for (auto & e : _march)
                {
                    double d = getDistance(e->_move.position, e->_control.spawnpoint);
                    EPosition dst = e->_control.spawnpoint;
                    if (d  < 2.0)
                    {
                        dst = (_marchOrg - dst)*2.0 + dst;
                    }
                    scene->_move->doMove(e->_state.eid, MOVE_ACTION_PATH, e->getSpeed(), 0, { dst });
                }
            }
        }
    }
}


void AI::rebirthCheck()
{
    auto scene = _scene.lock();
    if (!scene)
    {
        return;
    }
    SceneEventNotice eventNotice;
    for (auto kv : scene->_entitys)
    {
        if (kv.second->_state.state == ENTITY_STATE_LIE || kv.second->_state.state == ENTITY_STATE_DIED)
        {
            if (kv.second->_state.etype == ENTITY_FLIGHT)
            {
                scene->pushAsync(std::bind(&Scene::removeEntity, scene, kv.second->_state.eid));
            }
            else if (kv.second->_control.stateChageTime + 10.0 < getFloatSteadyNowTime())
            {
                kv.second->_state.state = ENTITY_STATE_ACTIVE;
                kv.second->_state.curHP = kv.second->_props.hp;
                kv.second->_isStateDirty = true;
                kv.second->_move.position = kv.second->_control.spawnpoint;
                if (scene->_move->isValidAgent(kv.second->_control.agentNo))
                {
                    scene->_move->setAgentPosition(kv.second->_control.agentNo, kv.second->_move.position);
                }
                SceneEventInfo ev;
                ev.src = InvalidEntityID;
                ev.dst = kv.second->_state.eid;
                ev.ev = SCENE_EVENT_REBIRTH;
                ev.val = kv.second->_state.curHP;
                mergeToString(ev.mix, ",", kv.second->_move.position.x);
                mergeToString(ev.mix, ",", kv.second->_move.position.y);
                eventNotice.info.push_back(ev);
            }
        }
    }
    if (!eventNotice.info.empty())
    {
        scene->broadcast(eventNotice);
    }

}




