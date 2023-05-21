import java.util.ArrayList;

/**
 * A class defining a TownPlan which will also store the solution to the police station problem.
 */
public class TownPlan {
    /**
     * Number of houses
     */
    private Integer n;

    /**
     * Number of police stations
     */
    private Integer k;

    /**
     * A list containing house positions in ascending order
     */
    private ArrayList<Integer> housePositions;

    /**
     * A list containing the optimal police station positions in ascending order,
     * which should be set by the findOptimalPoliceStationPositions method of Program3.java
     */
    private ArrayList<Integer> policeStationPositions;

    /**
     * The optimal response time for the TownPlan,
     * which should be set by findOptimalResponseTime method of Program3.java
     */ 
    private Integer responseTime;

    public TownPlan(Integer n, Integer k, ArrayList<Integer> housePositions) {
        this.n = n;
        this.k = k;
        this.housePositions = housePositions;
        this.policeStationPositions = null;
        this.responseTime = null;
    }

    public TownPlan(TownPlan data) {
        this(data.n, data.k, data.housePositions);
    }

    /**
     * Use this method to set "policeStationPositions" from the findOptimalPoliceStationPositions method of Program3.java
     */
    public void setPoliceStationPositions(ArrayList<Integer> policeStationPositions) {
        this.policeStationPositions = policeStationPositions;
    }

    /**
     * Use this method to set "responseTime" from the findOptimalResponseTime method of Program3.java
     */
    public void setResponseTime(Integer responseTime){
	    this.responseTime = responseTime;
    }

    public Integer getHouseCount() {
        return n;
    }

    public Integer getStationCount() {
        return k;
    }

    public ArrayList<Integer> getHousePositions() {
        return housePositions;
    }

    public ArrayList<Integer> getPoliceStationPositions() {
        return policeStationPositions;
    }

    public Integer getResponseTime() {
        return responseTime;
    }

    public String getInputSizeString() {
        return String.format("n = %d k = %d\n", n, k);
    }

    public String getResponseString(){
	    if (responseTime == null) {
            return "";
        }

        return String.format("Optimal response time =  %d\n", responseTime);
    }
    public String getPositionString() {
        if (policeStationPositions == null) {
            return "";
        }

        StringBuilder s = new StringBuilder();
        for (int i = 0; i < policeStationPositions.size(); i++) {
            String str = String.format("Station %d Position %d", i, policeStationPositions.get(i));
            s.append(str);
            if (i != policeStationPositions.size() - 1) {
                s.append("\n");
            }
        }
        return s.toString();
    }

    public String toString() {
        return getInputSizeString() + getResponseString() + getPositionString();
    }
}
