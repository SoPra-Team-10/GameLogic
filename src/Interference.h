/**
 * @file Interference.h
 * @author Björn, Tim
 * @date 05.05.19
 * @brief Declaration of the Interferences.
 */

#ifndef SOPRAGAMELOGIC_INTERFERENCE_H
#define SOPRAGAMELOGIC_INTERFERENCE_H

#include <memory>
#include <cmath>
#include <vector>
#include "GameController.h"
#include "GameModel.h"

namespace gameController{
    class Interference {
    public:
        Interference(std::shared_ptr<gameModel::Environment> env, std::shared_ptr<gameModel::Team> team,
                gameModel::InterferenceType type);

        /**
         * Executes the interference.
         * @return the results of the executed action as gameController::ActionCheckResult: Foul if Interference was detected,
         * Success otherwise
         */
        virtual auto execute() const -> gameController::ActionCheckResult = 0;

        /**
         * Checks if the interference is possible
         * @return true if possible, else false
         */
        virtual bool isPossible() const;

        /**
         * Gets the type of interference.
         * @return the type of the interference as gameModel::InterferenceType
         */
        auto getType() const -> gameModel::InterferenceType;

    protected:
        std::shared_ptr<gameModel::Environment> env;
        std::shared_ptr<gameModel::Team> team;
        gameModel::InterferenceType type;
    };

    class Teleport : public Interference {
    public:
        Teleport(std::shared_ptr<gameModel::Environment> env, std::shared_ptr<gameModel::Team> team,
                 std::shared_ptr<gameModel::Player> target);

        /**
         * Teleports target player to random free location on the field
         * @return see Interference::execute
         */
        auto execute() const -> gameController::ActionCheckResult override;

        /**
         * Checks if Interference is possible
         * @return true if available an target Player on field
         */
        bool isPossible() const override;

    private:
        std::shared_ptr<gameModel::Player> target;
    };

    class RangedAttack : public Interference {
    public:
        RangedAttack(std::shared_ptr<gameModel::Environment> env, std::shared_ptr<gameModel::Team> team,
                std::shared_ptr<gameModel::Player> target);

        /**
         * Pushes target player to a random free adjacent position
         * if target player previously held quaffle, quaffle will be moved to random free adjacent position
         * @return see Interference::execute
         */
        auto execute() const -> gameController::ActionCheckResult override;

        /**
         * Checks if Interference is possible
         * @return true if available and opponent target, false otherwise
         */
        bool isPossible() const override;
    private:
        std::shared_ptr<gameModel::Player> target;
    };

    class Impulse : public Interference {
    public:
        Impulse(std::shared_ptr<gameModel::Environment> env, std::shared_ptr<gameModel::Team> team);

        /**
         * If a Keeper or Chaser holds the quaffle, quaffle is moved to a random free adjacent position
         * @return see Interference::execute
         */
        auto execute() const -> gameController::ActionCheckResult override;

    };

    class SnitchPush : public Interference {
    public:
        SnitchPush(std::shared_ptr<gameModel::Environment> env, std::shared_ptr<gameModel::Team> team);

        /**
         * Snitch is moved to random free adjacent position
         * @return see Interference::execute
         */
        auto execute() const -> gameController::ActionCheckResult override;
    };

    class BlockCell : public Interference {
    public:
        BlockCell(std::shared_ptr<gameModel::Environment> env, std::shared_ptr<gameModel::Team> team, gameModel::Position target);

        /**
         * Checks if Interference is possible
         * @return true if this Interference is available and possible to place CubeOfShit on specified position
         */
        bool isPossible() const override ;

        /**
         * Places CubeOfShit on target position
         * @return see Interference::execute
         */
        auto execute() const -> gameController::ActionCheckResult override;
    private:
        gameModel::Position target;
    };
}


#endif //SOPRAGAMELOGIC_INTERFERENCE_H
