#include "practice_fixes.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CheckpointObject.hpp>
#include <Geode/modify/PlayLayer.hpp>

class $modify(GJBaseGameLayer) {

  static void onModify(auto& self) {
    if (!self.setHookPriority("GJBaseGameLayer::checkpointActivated", -1))
      log::warn("GJBaseGameLayer::checkpointActivated hook priority fail xD.");

    if (!self.setHookPriority("GJBaseGameLayer::processMoveActions", -1))
      log::warn("GJBaseGameLayer::processMoveActions hook priority fail xD.");

    if (!self.setHookPriority("GJBaseGameLayer::processMoveActionsStep", -1))
      log::warn("GJBaseGameLayer::processMoveActionsStep hook priority fail xD.");

    if (!self.setHookPriority("GJBaseGameLayer::processAreaMoveGroupAction", -1))
      log::warn("GJBaseGameLayer::processAreaMoveGroupAction hook priority fail xD.");
  }

  void toggleFlipped(bool p0, bool p1) {

  // This is just instant mirror portal
  // Didnt make it a setting cus its needed to fix a position bug when respawning on a checkpoint

    if (Global::get().state == state::recording)
      return GJBaseGameLayer::toggleFlipped(p0, true);

    return GJBaseGameLayer::toggleFlipped(p0, p1);

  }

  void checkpointActivated(CheckpointGameObject * cp) {

    if (Global::get().state == state::none) 
      GJBaseGameLayer::checkpointActivated(cp);
    else
      Global::get().cancelCheckpoint = true;
  }

  void processMoveActions() {
    if (!m_player1->m_isDead || Global::get().state == state::none)
      GJBaseGameLayer::processMoveActions();
  }
	void processMoveActionsStep(float v1, bool v2) {
    if (!m_player1->m_isDead || Global::get().state == state::none)
      GJBaseGameLayer::processMoveActionsStep(v1, v2);
  }

  void processAreaMoveGroupAction(cocos2d::CCArray* v1, EnterEffectInstance* v2, cocos2d::CCPoint v3, int v4, int v5, int v6, int v7, int v8, bool v9, bool v10) {
    if (!m_player1->m_isDead || Global::get().state == state::none)
      GJBaseGameLayer::processAreaMoveGroupAction(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10);
  }

};

class $modify(CheckpointObject) {
  #ifndef GEODE_IS_ANDROID
  bool init() {
    bool ret = CheckpointObject::init();
    CheckpointObject* cp = this;
  #else
  static CheckpointObject* create() {
    CheckpointObject* ret = CheckpointObject::create();
    CheckpointObject* cp = ret;
  #endif

    if (!cp) return ret;

    auto& g = Global::get();
    PlayLayer* pl = PlayLayer::get();

    PlayerData p1Data = PlayerPracticeFixes::saveData(pl->m_player1);
    PlayerData p2Data = PlayerPracticeFixes::saveData(pl->m_player2);

    Global::get().checkpoints[cp] = {
      Global::getCurrentFrame(),
      p1Data,
      p2Data,
      #ifdef GEODE_IS_WINDOWS
      *(uintptr_t*)((char*)geode::base::get() + seedAddr),
      #else
      0,
      #endif
      Global::get().previousFrame
    };

    return ret;

  }
};

class $modify(PlayLayer) {

  static void onModify(auto& self) {
    if (!self.setHookPriority("PlayLayer::storeCheckpoint", -1))
      log::warn("PlayLayer::storeCheckpoint hook priority fail xD.");
  }

  void storeCheckpoint(CheckpointObject * cp) {
    if (!cp) return PlayLayer::storeCheckpoint(cp);

    auto& g = Global::get();

    if (!g.cancelCheckpoint)
      PlayLayer::storeCheckpoint(cp);
    else {
      g.cancelCheckpoint = false;
      return;
    }

  }

  void loadFromCheckpoint(CheckpointObject* cp) {
    
    if (!cp) return PlayLayer::loadFromCheckpoint(cp);

    auto& g = Global::get();

    Macro::tryAutosave(m_level, cp);

    if (g.state == state::playing) {
      PlayLayer::loadFromCheckpoint(cp);

      if (!g.checkpoints.contains(cp)) return;
      if (g.checkpoints[cp].frame <= 1) return;

      PlayerData p1Data = g.checkpoints[cp].p1;
      PlayerData p2Data = g.checkpoints[cp].p2;

      g.respawnFrame = g.checkpoints[cp].frame;
      g.previousFrame = g.checkpoints[cp].previousFrame;
      PlayerPracticeFixes::applyData(this->m_player1, p1Data, false);
      PlayerPracticeFixes::applyData(this->m_player2, p2Data, true);

      return;
    }

    if ((g.state != state::recording && !Mod::get()->getSavedValue<bool>("macro_always_practice_fixes")))
      return PlayLayer::loadFromCheckpoint(cp);

    if (!g.checkpoints.contains(cp)) return PlayLayer::loadFromCheckpoint(cp);

    Macro::resetVariables();

    int frame = g.checkpoints[cp].frame;
    PlayerData p1Data = g.checkpoints[cp].p1;
    PlayerData p2Data = g.checkpoints[cp].p2;

    g.ignoreJumpButton = frame + 1;
    g.previousFrame = g.checkpoints[cp].previousFrame;

    #ifdef GEODE_IS_WINDOWS

    if (g.seedEnabled) {
      uintptr_t seed = g.checkpoints[cp].seed;
      *(uintptr_t*)((char*)geode::base::get() + seedAddr) = seed;
    }

    #endif

    if (g.state == state::recording)
      InputPracticeFixes::applyFixes(this, p1Data, p2Data, frame);

    PlayLayer::loadFromCheckpoint(cp);

    PlayerPracticeFixes::applyData(this->m_player1, p1Data, false);
    PlayerPracticeFixes::applyData(this->m_player2, p2Data, true);

    if (g.state != state::recording && g.mod->getSavedValue<bool>("macro_always_practice_fixes")) {
      this->m_player1->releaseButton(static_cast<PlayerButton>(1));
      this->m_player2->releaseButton(static_cast<PlayerButton>(1));
    }

  }

};
