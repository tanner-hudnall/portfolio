/*
 * Name: <your name>
 * EID: <your EID>
 */

// Implement your heap here
// Methods may be added to this file, but don't remove anything
// Include this file in your final submission

import java.util.ArrayList;

public class Heap {
    private ArrayList<Student> minHeap;

    public Heap() {
        minHeap = new ArrayList<Student>();
    }

    /**
     * buildHeap(ArrayList<Student> students)
     * Given an ArrayList of Students, build a min-heap keyed on each Student's minCost
     * Time Complexity - O(nlog(n)) or O(n)
     *
     * @param students
     */
    public void buildHeap(ArrayList<Student> students) {
        // Done: implement this method

        for(int i = 0; i < students.size(); i++){//create an unorganized heap with all students getting their indices 
            minHeap.add(students.get(i));
            minHeap.get(i).setIndex(i);
        }
        
        for(int i = (minHeap.size() / 2) - 1; i >= 0 ; i--){//start on the last parent node and heapify each parent so that it becomes organized
            heapify(i);
        }
    }

    /**
     * insertNode(Student in)
     * Insert a Student into the heap.
     * Time Complexity - O(log(n))
     *
     * @param in - the Student to insert.
     */
    public void insertNode(Student in) {
        // Done: implement this method
        minHeap.add(in);
        minHeap.get(minHeap.size() - 1).setIndex(minHeap.size() - 1);
        heapify(minHeap.size() - 1);
    }

    /**
     * findMin()
     * Time Complexity - O(1)
     *
     * @return the minimum element of the heap.
     */
    public Student findMin() {
        // Done: implement this method
        return minHeap.get(0);
    }

    /**
     * extractMin()
     * Time Complexity - O(log(n))
     *
     * @return the minimum element of the heap, AND removes the element from said heap.
     */
    public Student extractMin() {
        // Done: implement this method
        if(!minHeap.isEmpty()){
            Student s = findMin();
            delete(0);
            return s;
        }
        else{
            return null;
        }
    }

    /**
     * delete(int index)
     * Deletes an element in the min-heap given an index to delete at.
     * Time Complexity - O(log(n))
     *
     * @param index - the index of the item to be deleted in the min-heap.
     */
    public void delete(int index) {
        // Done: implement this method
        if(!minHeap.isEmpty()){
            swap(index, minHeap.size() - 1);
            minHeap.remove(minHeap.size() - 1);
            heapify(index);
        }
        
    }

    /**
     * changeKey(Student r, int newCost)
     * Changes minCost of Student s to newCost and updates the heap.
     * Time Complexity - O(log(n))
     *
     * @param r       - the Student in the heap that needs to be updated.
     * @param newCost - the new cost of Student r in the heap (note that the heap is keyed on the values of minCost)
     */
    public void changeKey(Student r, int newCost) {
        // Done: implement this method
        minHeap.get(r.getIndex()).setminCost(newCost);
        heapify(r.getIndex());
    }

    public String toString() {
        String output = "";
        for (int i = 0; i < minHeap.size(); i++) {
            output += minHeap.get(i).getName() + " ";
        }
        return output;
    }

    public void heapify(int index){//by some error in code, this code just be 2*log(n), which is still O(log(n))
        heapifyUp(index); //should not change anything if you need to heapify down
        heapifyDown(index); //should not change anything if you need to heapify up
    }

    public void heapifyUp(int index){
        if(index == 0){
            return;
        }

        int parent = (index - 1) / 2;
        if(minHeap.get(index).getminCost() < minHeap.get(parent).getminCost()){
            swap(index, parent);
            heapifyUp(parent);
        }
        else if(minHeap.get(index).getminCost() == minHeap.get(parent).getminCost()){
            if(minHeap.get(index).getName() < minHeap.get(parent).getName()){
                swap(index, parent);
                heapifyUp(parent);
            }
        }
    }

    public void heapifyDown(int index){
        int left = (2 * index) + 1; //left child
        int right = (2 * index) + 2; //right child
        int least = index;

        if(left < minHeap.size() && minHeap.get(left).getminCost() < minHeap.get(least).getminCost()){ //if left exists and is the index of a "cheaper" student than index
            least = left;
        }
        else if(left < minHeap.size() && minHeap.get(left).getminCost() == minHeap.get(least).getminCost()){
            if(minHeap.get(left).getName() < minHeap.get(least).getName()){
                least = left;
            }
        }
        if(right < minHeap.size() && minHeap.get(right).getminCost() < minHeap.get(least).getminCost()){ //if right exists and is the index of a "cheaper" student than index or left if it swapped
            least = right;
        }
        else if(right < minHeap.size() && minHeap.get(right).getminCost() == minHeap.get(least).getminCost()){
            if(minHeap.get(right).getName() < minHeap.get(least).getName()){
                least = right;
            }
        }

        if(least != index){ //if either of the children were less than the parent
            swap(least, index); //swap the child and the parent in the arraylist
            heapifyDown(least); //call buildMinHeap with the index least, which now contains the student that WAS the parent but just got swapped
        }
    }

    public void swap(int index1, int index2){
        Student student1 = minHeap.get(index1);
        Student student2 = minHeap.get(index2);
        int tempIndex = student2.getIndex();
        minHeap.set(student2.getIndex(), student1);
        minHeap.set(student1.getIndex(), student2);
        student2.setIndex(student1.getIndex());
        student1.setIndex(tempIndex);
    }

    public Student get(int index){
        if((index > -1) && (index < minHeap.size())){
            return minHeap.get(index);
        }
        else{
            return null;
        }
    }

    public int size(){
        return minHeap.size();
    }

   

///////////////////////////////////////////////////////////////////////////////
//                           DANGER ZONE                                     //
//                everything below is used for grading                       //
//                      please do not change :)                              //
///////////////////////////////////////////////////////////////////////////////

    public ArrayList<Student> toArrayList() {
        return minHeap;
    }
}
