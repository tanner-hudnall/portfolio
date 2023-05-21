/*
 * Name: <your name>
 * EID: <your EID>
 */

// Implement your algorithms here
// Methods may be added to this file, but don't remove anything
// Include this file in your final submission

import java.util.ArrayList;

public class Program2 {
    private ArrayList<Student> students;    // this is a list of all Students, populated by Driver class
    private Heap minHeap;

    // additional constructors may be added, but don't delete or modify anything already here
    public Program2(int numStudents) {
        minHeap = new Heap();
        students = new ArrayList<Student>();
    }

    /**
     * findMinimumStudentCost(Student start, Student dest)
     *
     * @param start - the starting Student.
     * @param dest  - the end (destination) Student.
     * @return the minimum cost possible to get from start to dest.
     * Assume the given graph is always connected.
     */
    public int findMinimumStudentCost(Student start, Student dest) { 
        // Done: implement this function
        initialize(start);
        while(minHeap.size() != 0 ){
            Student u = extraction();
            if(u.compareTo(dest) == 0){
                return u.getminCost();
            }
            for(int i = 0; i < u.getNeighbors().size(); i++){
                int newMin = u.getminCost() + u.getPrices().get(i); //newMin is cost to get there + cost to get from u to neighbor
                Student v = u.getNeighbors().get(i);
                update(v, newMin);
            }
        }
        return -1;
    }

    /**
     * findMinimumClassCost()
     *
     * @return the minimum total cost required to connect (span) each student in the class.
     * Assume the given graph is always connected.
     */
    public int findMinimumClassCost() {
        // Done: implement this function
        int spanningTreeCost = 0;
        initialize(null);
        while(minHeap.size() != 0){
            Student u = extraction();
            spanningTreeCost += u.getminCost();
            for(int i = 0; i < u.getNeighbors().size(); i++){
                Student v = u.getNeighbors().get(i);
                int pathCost = u.getPrices().get(i);
                update(v, pathCost);
            }
        }
        return spanningTreeCost;
    }

    public Student extraction(){
        Student removed = minHeap.extractMin();
        removed.setVisited(true);
        return removed;
    }

    public void initialize(Student s){
        for(int i = 0; i < students.size(); i++){
            students.get(i).resetminCost();
            students.get(i).setIndex(i);
        }
        if(s == null){
            students.get(0).setminCost(0);
        }
        else{
            students.get(s.getIndex()).setminCost(0);
        }
        minHeap.buildHeap(students);
    }

    public void update(Student v, int comparator){
        if((!v.getVisited()) && (comparator < v.getminCost())){
            v.setminCost(comparator);
            minHeap.changeKey(v, comparator);
        }
    }

    //returns edges and prices in a string.
    public String toString() {
        String o = "";
        for (Student v : students) {
            boolean first = true;
            o += "Student ";
            o += v.getName();
            o += " has neighbors ";
            ArrayList<Student> ngbr = v.getNeighbors();
            for (Student n : ngbr) {
                o += first ? n.getName() : ", " + n.getName();
                first = false;
            }
            first = true;
            o += " with prices ";
            ArrayList<Integer> wght = v.getPrices();
            for (Integer i : wght) {
                o += first ? i : ", " + i;
                first = false;
            }
            o += System.getProperty("line.separator");

        }

        return o;
    }

   

///////////////////////////////////////////////////////////////////////////////
//                           DANGER ZONE                                     //
//                everything below is used for grading                       //
//                      please do not change :)                              //
///////////////////////////////////////////////////////////////////////////////

    public Heap getHeap() {
        return minHeap;
    }

    public ArrayList<Student> getAllstudents() {
        return students;
    }

    // used by Driver class to populate each Student with correct neighbors and corresponding prices
    public void setEdge(Student curr, Student neighbor, Integer price) {
        curr.setNeighborAndPrice(neighbor, price);
    }

    // used by Driver.java and sets students to reference an ArrayList of all Students
    public void setAllNodesArray(ArrayList<Student> x) {
        students = x;
    }
}
