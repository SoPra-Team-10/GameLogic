#include <random>
#include <deque>
#include "GameController.h"

namespace gameController {

    template <>
    double rng(double min, double max){
        static std::random_device rd;
        static std::default_random_engine el(rd());
        std::uniform_real_distribution dist(min, max);
        return dist(el);
    }

    template <>
    int rng(int min, int max){
        static std::random_device rd;
        static std::default_random_engine el(rd());
        std::uniform_int_distribution dist(min, max);
        return dist(el);
    }

    bool actionTriggered(double actionProbability) {
        if(actionProbability < 0 || actionProbability > 1){
            throw std::invalid_argument("Probability not between 0 an 1");
        }

        return rng(0.0, 1.0) < actionProbability;
    }

    auto getAllCrossedCells(const gameModel::Position &startPoint, const gameModel::Position &endPoint) ->
        std::vector<gameModel::Position> {

        std::vector<gameModel::Position> resultVect;

        // check if cells are valid
        if (gameModel::Environment::getCell(startPoint) == gameModel::Cell::OutOfBounds ||
        gameModel::Environment::getCell(endPoint) == gameModel::Cell::OutOfBounds){
            throw std::out_of_range("Source or destination of movement vector are out of bounds");
        }

        // check if start and end point are equal
        if (startPoint == endPoint)
            return resultVect;

        // define an normalize the direction vector
        gameModel::Vector dirVect(endPoint.x - startPoint.x, endPoint.y - startPoint.y);
        dirVect.normalize();

        // traverse the route between the two points
        gameModel::Vector travVect(0, 0);
        gameModel::Position lastCell = startPoint;
        while ((travVect + startPoint) != endPoint) {

            // found a new crossed cell
            if ((travVect + startPoint) != lastCell) {
                lastCell = travVect + startPoint;
                resultVect.emplace_back(lastCell);
            }

            // make a step to travers the vector
            travVect = travVect + (dirVect * 0.5);
        }

        return resultVect;
    }

    auto getDistance(const gameModel::Position &startPoint, const gameModel::Position &endPoint) -> int {

        // check if cells are valid
        if (gameModel::Environment::getCell(startPoint) == gameModel::Cell::OutOfBounds ||
            gameModel::Environment::getCell(endPoint) == gameModel::Cell::OutOfBounds){

            return -1;
        }

        // check if start and end point are equal
        if (startPoint == endPoint){
            return 0;
        }

        int totalDistance = 0;

        // calc the differences within the components of the given points
        int dX = abs(startPoint.x - endPoint.x);
        int dY = abs(startPoint.y - endPoint.y);

        // calculate the total distance
        if (dX >= dY) {
            totalDistance += dY;
            totalDistance += dX-dY;
        } else {
            totalDistance += dX;
            totalDistance += dY-dX;
        }

        return totalDistance;
    }

    void moveToAdjacent(const std::shared_ptr<gameModel::Object> &object, const std::shared_ptr<gameModel::Environment> &env) {
        auto positions = env->getAllPlayerFreeCellsAround(object->position);
        object->position = positions[rng(0, static_cast<int>(positions.size()) - 1)];
    }

    auto getAllPossibleMoves(std::shared_ptr<gameModel::Player>, const gameModel::Environment&) -> std::vector<Move> {
        return std::vector<Move>();
        // @ToDo: Nicht für Server relevant
    }

    auto getAllPossibleShots(std::shared_ptr<gameModel::Player>,
                             const gameModel::Environment&) -> std::vector<Shot> {
        return std::vector<Shot>();
        // @ToDo: Nicht für Server relevant
    }

    auto refereeDecision(const gameModel::Foul &foul, const gameModel::Config &gameConf) -> bool {
        switch (foul) {
            case gameModel::Foul::MultipleOffence :
                return actionTriggered(gameConf.foulDetectionProbs.multipleOffence);
            case gameModel::Foul::ChargeGoal :
                return actionTriggered(gameConf.foulDetectionProbs.chargeGoal);
            case gameModel::Foul::BlockGoal :
                return actionTriggered(gameConf.foulDetectionProbs.blockGoal);
            case gameModel::Foul::Ramming :
                return actionTriggered(gameConf.foulDetectionProbs.ramming);
            case gameModel::Foul::BlockSnitch :
                return actionTriggered(gameConf.foulDetectionProbs.blockSnitch);
            default :
                return  false;
        }
    }

    // ToDo: TESTS!!!
    auto moveBludger(std::shared_ptr<gameModel::Bludger> &bludger, std::shared_ptr<gameModel::Environment> &env)
        -> std::optional<std::shared_ptr<gameModel::Player>> {
        auto players = env->getAllPlayers();

        // find nearest player
        int minDistance = std::numeric_limits<int>::max();
        std::shared_ptr<gameModel::Player> minDistancePlayer;
        for (const auto &player: players) {

            if (!INSTANCE_OF(player, gameModel::Beater)) {
                if (getDistance(bludger->position, player->position) < minDistance) {
                    minDistance = getDistance(bludger->position, player->position);
                    minDistancePlayer = player;
                }
            }
        }

        if (!minDistancePlayer) {
            throw std::runtime_error("There are't enough player on the field!");
        }

        // move towards nearest player
        auto crossedCells = getAllCrossedCells(bludger->position, minDistancePlayer->position);
        if (crossedCells.empty()) {
            // bludger will move on the players position
            bludger->position = minDistancePlayer->position;

            // roll the dices
            if (actionTriggered(env->config.gameDynamicsProbs.knockOut)) {
                // quaffel test
                if (env->quaffle->position == minDistancePlayer->position) {
                    moveToAdjacent(env->quaffle, env);
                }
                // knockout player
                minDistancePlayer->knockedOut = true;

                //Set Bludger to new random position
                auto possiblePositions = env->getAllFreeCells();
                bludger->position = possiblePositions[rng(0, static_cast<int>(possiblePositions.size() - 1))];
            }

            return minDistancePlayer;
        }
        else {
            // move in the direction of the nearest player
            bludger->position = crossedCells[0];
        }

        return {};
    }

    bool playerCanPerformAction(const std::shared_ptr<const gameModel::Player> &player,
                                const std::shared_ptr<const gameModel::Environment> &env) {
        if(player->knockedOut || player->isFined){
            return false;
        }

        // check if there is generally allowed to perform a shot and a ball to shot on the same position as the player
        if(player->position == env->quaffle->position && (INSTANCE_OF(player, const gameModel::Chaser) ||
            INSTANCE_OF(player, const gameModel::Keeper))){
            return true;
        } else if((player->position == env->bludgers[0]->position || player->position == env->bludgers[1]->position) &&
                    INSTANCE_OF(player, const gameModel::Beater)){
            return true;
        }

        // check if player can wrest quaffel
        auto playerHoldingQuaffle = env->getPlayer(env->quaffle->position);
        if(INSTANCE_OF(player, const gameModel::Chaser) && getDistance(player->position, env->quaffle->position) == 1 &&
            playerHoldingQuaffle.has_value() && (INSTANCE_OF(playerHoldingQuaffle.value(), gameModel::Chaser) ||
                (INSTANCE_OF(playerHoldingQuaffle.value(), gameModel::Keeper) && !env->isPlayerInOwnRestrictedZone(playerHoldingQuaffle.value())))){
            return true;
        }

        return false;
    }

    void moveSnitch(std::shared_ptr<gameModel::Snitch> &snitch, std::shared_ptr<gameModel::Environment> &env, ExcessLength excessLength){
        if (!snitch->exists) {
            throw std::runtime_error("Snitch does not exist");
        }
        std::deque<gameModel::Position> possiblePositions;
        switch (excessLength) {
            case ExcessLength::None : {
                int minDistanceSeeker = getDistance(snitch->position, env->team1->seeker->position);
                auto closestSeeker = env->team1->seeker;
                auto distanceTeam2 = getDistance(snitch->position, env->team2->seeker->position);
                std::vector<gameModel::Position> freeCells = env->getAllPlayerFreeCellsAround(snitch->position);
                if(minDistanceSeeker == distanceTeam2) {
                    for (const auto &pos : freeCells) {
                        if (getDistance(pos, closestSeeker->position) == getDistance(pos, env->team2->seeker->position)) {
                            possiblePositions.emplace_back(pos);
                        }
                    }
                }else if (distanceTeam2 < minDistanceSeeker) {
                    minDistanceSeeker = distanceTeam2;
                    closestSeeker = env->team2->seeker;
                    for (const auto &pos : freeCells) {
                        if (getDistance(pos, closestSeeker->position) > minDistanceSeeker) {
                            possiblePositions.emplace_back(pos);
                        }
                    }
                }else{
                    for (const auto &pos : freeCells) {
                        if (getDistance(pos, closestSeeker->position) > minDistanceSeeker) {
                            possiblePositions.emplace_back(pos);
                        }
                    }
                }
                if (possiblePositions.empty()) {
                    snitch->position = freeCells[rng(0, static_cast<int>(freeCells.size() - 1))];
                } else {
                    snitch->position = possiblePositions[rng(0, static_cast<int>(possiblePositions.size() - 1))];
                }
            }
            break;
            case ExcessLength::Stage1:
                break;
            case ExcessLength::Stage2: {
                std::vector<gameModel::Position> newPosition = getAllCrossedCells(snitch->position,
                                                                                  gameModel::Position(8, 6));
                if (newPosition.empty()) {
                    snitch->position = gameModel::Position{8,6};
                }else{
                    snitch->position = newPosition[0];
                }
            }break;
            case ExcessLength::Stage3: {
                int minDistanceSeeker = getDistance(snitch->position, env->team1->seeker->position);
                auto closestSeeker = env->team1->seeker;
                auto distanceTeam2 = getDistance(snitch->position, env->team2->seeker->position);
                if (distanceTeam2 < minDistanceSeeker) {
                    closestSeeker = env->team2->seeker;
                }
                snitch->position = closestSeeker->position;
                env->getTeam(closestSeeker)->score += 30;
            }break;
        }
    }
}