/*
 * Name: <Tanner Hudnall>
 * EID: <tch2368>
 */

import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedList;

/**
 * Your solution goes in this class.
 * 
 * Please do not modify the other files we have provided for you, as we will use
 * our own versions of those files when grading your project. You are
 * responsible for ensuring that your solution works with the original version
 * of all the other files we have provided for you.
 * 
 * That said, please feel free to add additional files and classes to your
 * solution, as you see fit. We will use ALL of your additional files when
 * grading your solution.
 */
public class Program3 extends AbstractProgram3 {
    int[][] maxResponseTime;
    ArrayList<ArrayList<ArrayList<Integer>>> policePositions;

    /**
     * Determines the solution of the optimal response time for the given input TownPlan. Study the
     * project description to understand the variables which represent the input to your solution.
     *
     * @return Updated TownPlan town with the "responseTime" field set to the optimal response time
     */
    @Override
    public TownPlan findOptimalResponseTime(TownPlan town) {
        /* TODO implement this function */
        maxResponseTime = new int[town.getHouseCount()][town.getStationCount()];
        for(int i = 0; i < maxResponseTime.length; i++){
            maxResponseTime[i][0] = (town.getHousePositions().get(i) - town.getHousePositions().get(0)) / 2;
        }
        
        for(int i = 0; i < maxResponseTime[0].length; i++){
            maxResponseTime[0][i] = 0;
        }

        for(int a = 1; a < maxResponseTime[0].length; a++){
            for(int b = 1; b < maxResponseTime.length; b++){
                maxResponseTime[b][a] = helperFunction(town, b, a, true);
            }
        }

        town.setResponseTime(maxResponseTime[maxResponseTime.length - 1][maxResponseTime[0].length - 1]);
        return town;
    }

    /**
     * Determines the solution of the set of police station positions that optimize response time for the given input TownPlan. Study the
     * project description to understand the variables which represent the input to your solution.
     *
     * @return Updated TownPlan town with the "policeStationPositions" field set to the optimal police station positions
     */
    @Override
    public TownPlan findOptimalPoliceStationPositions(TownPlan town) {
        /* TODO implement this function */
        town = findOptimalResponseTime(town);
        policePositions = new ArrayList<>();
        for(int i = 0; i < town.getHouseCount(); i++){
            policePositions.add(new ArrayList<>()); //add the house
            for(int k = 0; k < town.getStationCount(); k++){
                policePositions.get(i).add(new ArrayList<>()); //add the station counts
            }
        }
        for(int i = 0; i < town.getHouseCount(); i++){
            policePositions.get(i).get(0).add((town.getHousePositions().get(i) + town.getHousePositions().get(0)) / 2);
        }
        
        for(int i = 1; i < town.getStationCount(); i++){
            policePositions.get(0).get(i).add(town.getHousePositions().get(0));
        }

        for(int a = 1; a < town.getStationCount(); a++){
            for(int b = 1; b < town.getHouseCount(); b++){
                housePositionHelper(town, b, a);
            }
        }




        town.setPoliceStationPositions(policePositions.get(town.getHouseCount() - 1).get(town.getStationCount() - 1));
        return town;
    }

    private int helperFunction(TownPlan town, int currentHouse, int currentStation, boolean mode){
        int iValue, responseTime, previousSolution, gap;
        ArrayList<Integer> candidates = new ArrayList<>();
        for(int i = 1; i <= currentHouse; i++){
            previousSolution = maxResponseTime[currentHouse - i][currentStation - 1];
            gap = (town.getHousePositions().get(currentHouse) - town.getHousePositions().get(currentHouse - i)) / 2;
            if(previousSolution < gap){
                candidates.add(previousSolution);
            }
            else{
                candidates.add(gap);
            }
        }

        responseTime = candidates.get(0);
        iValue = 1;
        
        for(int i = 1; i < candidates.size(); i++){
            if(candidates.get(i) > responseTime){
                responseTime = candidates.get(i);
                iValue = i + 1;
            }
        }

        if(mode){
            return responseTime;
        }
        else{
            return iValue;
        }
    }

    private void housePositionHelper(TownPlan town, int currentHouse, int currentStation){
        int iValue, previousSolution, gap, ender;
        ArrayList<Integer> answerArray = new ArrayList<Integer>();
        iValue = helperFunction(town, currentHouse, currentStation, false);
        previousSolution = maxResponseTime[currentHouse - iValue][currentStation - 1];
        gap = (town.getHousePositions().get(currentHouse) - town.getHousePositions().get(currentHouse - iValue)) / 2;
        if(previousSolution < gap){
            answerArray.addAll(policePositions.get(currentHouse - iValue).get(currentStation - 1));
            ender = (town.getHousePositions().get(currentHouse) + town.getHousePositions().get(currentHouse + 1 - iValue)) / 2;
            answerArray.add(ender);
        }
        else{
            answerArray.addAll(policePositions.get(currentHouse - 1 - iValue).get(currentStation - 1));
            ender = (town.getHousePositions().get(currentHouse) + town.getHousePositions().get(currentHouse - iValue)) / 2;
            answerArray.add(ender);
        }

        policePositions.get(currentHouse).get(currentStation).addAll(answerArray);
        
       // return tester;
    }
}
