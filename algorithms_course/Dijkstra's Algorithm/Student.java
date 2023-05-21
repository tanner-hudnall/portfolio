/*
 * Name: <your name>
 * EID: <your EID>
 */

// Methods may be added to this file, but don't remove anything
// Include this file in your final submission

import java.util.*;

public class Student {
    private int minCost;
    private int name;
    private ArrayList<Student> neighbors;
    private ArrayList<Integer> prices;
    private int index;
    private boolean visited;
    private Student predecessor;

    public Student(int x) {
        name = x;
        minCost = Integer.MAX_VALUE;
        neighbors = new ArrayList<Student>();
        prices = new ArrayList<Integer>();
    }

    public void setNeighborAndPrice(Student n, Integer w) {
        neighbors.add(n);
        prices.add(w);
    }

    public ArrayList<Student> getNeighbors() {
        return neighbors;
    }

    public ArrayList<Integer> getPrices() {
        return prices;
    }

    public int getminCost() { return minCost; }

    public void setminCost(int x) {
        minCost = x;
    }

    public void resetminCost() {
        minCost = Integer.MAX_VALUE;
    }

    public int getName() {
        return name;
    }

    public int getIndex(){
        return index;
    }

    public void setIndex(int i){
        index = i;
    }

    public boolean getVisited(){
        return visited;
    }

    public void setVisited(boolean visit){
        visited = visit;
    }

    public Student getPredecessor(){
        return predecessor;
    }

    public void setPredecessor(Student s){
        predecessor = s;
    }

    public int compareTo(Student comparator){
        if(this.getminCost() == comparator.getminCost()){
            if(this.getName() > comparator.getName()){
                return 1;
            }
            else if(this.getName() < comparator.getName()){
                return -1;
            }
            else{
                return 0;
            }
        }
        else if(this.getminCost() > comparator.getminCost()){
            return 1;
        }
        else{
            return -1;
        }
    }
}
