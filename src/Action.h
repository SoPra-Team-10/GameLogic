/**
 * @file Action.h
 * @author Björn, Jonas, Tim
 * @date
 * @brief Declaration of multiple classes for game actions.
 */

#ifndef SOPRAGAMELOGIC_SOPRAGAMEACTION_H
#define SOPRAGAMELOGIC_SOPRAGAMEACTION_H

#include <memory>
#include <cmath>
#include <vector>
#include "GameController.h"
#include "GameModel.h"

#define INSTANCE_OF(A, B) (std::dynamic_pointer_cast<B>(A))

namespace gameController{

    /**
     * Basic categories of Actions
     */
    enum class ActionCheckResult {
        Impossible, ///< Action violates game mechanics
        Success, ///< Action is possible without risk of a foul
        Foul ///< Action may result in a foul
    };

    /**
     * Outcomes of an Action
     */
    enum class ActionResult {
        Intercepted, ///<Quaffle is intercepted
        Miss, ///<Quaffle does not land on target position
        ScoreRight, ///<Throw resulted in a goal for the right team
        ScoreLeft, ///<Throw resulted in a goal for the left team
        ThrowSuccess, ///<Quaffle landed on target position
        Knockout, ///<Bludger knocked out a player
        SnitchCatch, ///Snitch is caught
        WrestQuaffel, ///>Quaffel was wrested
        FoolAway ///> Quaffle was lost
    };

    class Action {
    public:

        // constructors
        /**
         * default constructor for the Action class.
         */
        Action() = default;

        /**
         * main constructor for the Action class.
         * @param env The environment to operate on
         * @param actor the actor
         * @param target the target position
         */
        Action(std::shared_ptr<gameModel::Environment> env, std::shared_ptr<gameModel::Player> actor,
                gameModel::Position target);

        // destructors
        /**
         * default destructors for the Action class.
         */
        virtual ~Action() = default;

        // functions
        /**
         * executes the Action. Method is not deterministic.
         * @throws std::runtime_exception if Action is impossible
         * @return List of ramifications from the action execution, list of fouls during the execution
         */
        virtual auto execute() const -> std::pair<std::vector<ActionResult>, std::vector<gameModel::Foul>> = 0;

        /**
         * Calculates the probability of a successful execution
         */
        virtual auto successProb() const -> double = 0;

        /**
         * Checks if the Action is possible an if it may result in a foul
         * @return See ActionCheckResult
         */
        virtual auto check() const -> ActionCheckResult = 0;

        /**
         * Produces a list with all possible outcomes of the Action and the respective transition probabilities.
         * @return List of pairs consisting of the resulting Environment and the probability of landing in that state
         */
        virtual auto executeAll() const ->
            std::vector<std::pair<std::shared_ptr<gameModel::Environment>, double>> = 0;

        /**
         * Getter
         * @return target position of the Action
         */
        const gameModel::Position &getTarget() const;
    protected:
        // objects
        std::shared_ptr<gameModel::Player> actor;
        std::shared_ptr<gameModel::Environment> env;
        gameModel::Position target{};
    };

    /**
     * Represents actions with the game's balls except for the Snitch
     */
    class Shot : public Action {
    public:

        // constructors
        /**
         * main constructor for the Shot class.
         * @param env the environment to operate on
         * @param actor the acting player as shared pointer.
         * @param ball The ball to be moved
         * @param target the target position of the shot.
         */
        Shot(std::shared_ptr<gameModel::Environment> env, std::shared_ptr<gameModel::Player> actor,
                std::shared_ptr<gameModel::Ball> ball, gameModel::Position target);

        // functions
        auto execute() const -> std::pair<std::vector<ActionResult>, std::vector<gameModel::Foul>>;
        /**
         * When throwing Quaffle, returns probability that ball lands on target.
         * When shooting Bludger, returns probability of knocking out player on target.
         * @return probability for the shot to succeed
         */
        auto successProb() const -> double override;
        auto check() const -> ActionCheckResult override;
        auto executeAll() const ->
            std::vector<std::pair<std::shared_ptr<gameModel::Environment>, double>> override;
        /**
         * Checks if the defined Shot will result in a goal if it succeeds
         * @return an ActionResult with the appropriate message or nothing if no goal will be scored
         */
        auto isShotOnGoal() const -> std::optional<ActionResult>;

        /**
         * Returns the type of Shot as DeltaType
         * @return either DeltaType::QUAFFLE_THROW, DeltaType::BLUDGER_BEATING, or nothing
         */
        auto shotType() const -> std::optional<communication::messages::types::DeltaType>;

        /**
         * Getter
         * @return the ball of the Shot
         */
        const std::shared_ptr<const gameModel::Ball> getBall() const;
    private:
        std::shared_ptr<gameModel::Ball> ball;

        /**
         * Gets all cells along the flightpath which are occupied by opponent players (ordered in flight direction)
         * @return list with positions where the Quaffle might be intercepted
         */
        auto getInterceptionPositions() const -> std::vector<gameModel::Position>;

        /**
         * Returns a list with all possible positions the ball might land if NOT intercepted. The target position is NOT
         * included
         * @return list with all possible landing positions
         */
        auto getAllLandingCells() const -> std::vector<gameModel::Position>;

        /**
         * Checks if a goal was scored depending on the actors current position
         * @param pos the position where the quaffle was thrown
         * @return true if a successful throw from the actors position to pos counts as goal, false otherwise
         */
        auto goalCheck(const gameModel::Position &pos) const -> std::optional<ActionResult>;

        /**
         * creates all Environments for Quaffle throws
         * @return see Action::executeAll
         */
        auto executeAllQuaffle() const -> std::vector<std::pair<std::shared_ptr<gameModel::Environment>, double>>;

        /**
         * creates all Environments for Bludger shots
         * @return see Action::executeAll
         */
        auto executeAllBludger() const -> std::vector<std::pair<std::shared_ptr<gameModel::Environment>, double>>;

        /**
         * emplaces new Envs in return-list where the Quaffle landed on a cell in newPoses
         * and makes sure that no duplicate envs are created.
         * @param baseProb the probability that the Quaffle reached any of the cells in newPoses
         * @param newPoses positions for new envs
         * @param envList list where new envs and their corresponding probabilities are constructed
         */
        void emplaceEnvs(double baseProb, const std::vector<gameModel::Position> &newPoses,
                std::vector<std::pair<std::shared_ptr<gameModel::Environment>, double>> &envList) const;
    };

    /**
     * class for a the action of wresting the Quaffel from another player.
     */
    class WrestQuaffle : public Action {
    public:

        // constructors
        /**
         * default constructor for the WrestQuaffle class.
         */
        WrestQuaffle() = default;

        /**
         * main constructor for the WrestQuaffle class.
         * @param env the environment to operate on
         * @param actor the acting player (only chaser) as shared pointer.
         * @param target the target position of the shot.
         */
        WrestQuaffle(std::shared_ptr<gameModel::Environment> env, std::shared_ptr<gameModel::Chaser> actor,
                     gameModel::Position target);

        // functions
        auto execute() const -> std::pair<std::vector<ActionResult>, std::vector<gameModel::Foul>>;

        /**
         * Probability that the quaffle is wrested
         * @return the success probability of wresting the quaffel as double.
         */
        auto successProb() const -> double override;
        auto check() const -> ActionCheckResult override;
        auto executeAll() const ->
            std::vector<std::pair<std::shared_ptr<gameModel::Environment>, double>> override;

    };

    /**
     * class for a move in the game
     */
    class Move : public Action{
    public:

        // constructors
        /**
         * default constructor for the Move Class.
         */
        Move() = default;
        /**
         * main constructor for the Move class.
         * @param env the environment to operate on
         * @param actor the acting player or ball as shared pointer.
         * @param target the target position of the move.
         */
        Move(std::shared_ptr<gameModel::Environment> env, std::shared_ptr<gameModel::Player> actor,
                gameModel::Position target);

        // functions
        auto execute() const -> std::pair<std::vector<ActionResult>, std::vector<gameModel::Foul>> override;

        /**
         * Probability that the actor will land on target cell and is not banned for commiting a foul
         * @return the success probability of the move as double.
         */
        auto successProb() const -> double override;
        auto check() const -> ActionCheckResult override;
        auto executeAll() const ->
            std::vector<std::pair<std::shared_ptr<gameModel::Environment>, double>> override;

        /**
         * checks if the move is a foul.
         * @return List with possible fouls resulting from the move
         */
        auto checkForFoul() const -> std::vector<gameModel::Foul>;

    private:
        enum class ActionState {
            HandleFouls,
            MovePlayers,
            HandleBalls
        };

        void executePartially(std::vector<std::pair<std::shared_ptr<gameModel::Environment>, double>> &resList,
                ActionState state) const;
    };

}

#endif //SOPRAGAMELOGIC_SOPRAGAMEACTION_H
