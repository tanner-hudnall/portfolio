/*
 * Tanner Hudnall
 * tch2368
 */

import java.util.ArrayList;
//import java.util.Arrays;
//import java.util.LinkedList;
//import java.util.prefs.Preferences;

/*
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
public class Program1 extends AbstractProgram1 {


    /**
     * Determines whether a candidate Matching represents a solution to the stable matching problem.
     * Study the description of a Matching in the project documentation to help you with this.
     */
    @Override
    public boolean isStableMatching(Matching problem) {
        /*
         * An unstable pairing in this sense means that there is at least one pairing
         * such that a student prefers another school, and that school prefers them to 
         * at least one of their students
         */


    	for(int i = 0; i < problem.getStudentMatching().size(); i++) {
    	
    		ArrayList<Integer> preferencesOfI = problem.getStudentPreference().get(i);//preference list of student i
			int schoolOfI = problem.getStudentMatching().get(i);
			int indexOfSchool = preferencesOfI.indexOf(schoolOfI);
			
			for(int a = 0; a < indexOfSchool; a++){
				int testSchool = preferencesOfI.get(a);
				ArrayList<Integer> testSchoolPreference = problem.getHighSchoolPreference().get(testSchool);
				ArrayList<Integer> studentList = studentsAtSchool(problem.getStudentMatching(), testSchool);
				for(int b = 0; b < studentList.size(); b++){
					int test = studentList.get(b);
					if(testSchoolPreference.indexOf(i) < testSchoolPreference.indexOf(test)){
						return false;
					}
				}

			}

 		}
		return true;

	}

    /**
     * Determines a solution to the stable matching problem from the given input set. Study the
     * project description to understand the variables which represent the input to your solution.
     *
     * @return A stable Matching.
     */
    @Override
	public Matching stableMatchingGaleShapley_studentoptimal(Matching problem) {
		//make all temporary variables and rankMap
		ArrayList<Integer> matchingArray = new ArrayList<Integer>();
		for(int i = 0; i < problem.getStudentCount(); i++){
			matchingArray.add(-1);
		}
		
		ArrayList<ArrayList<Integer>> schoolPreference = new ArrayList<ArrayList<Integer>>();
		for(int i = 0; i < problem.getHighSchoolPreference().size(); i++){
			ArrayList<Integer> filler = new ArrayList<Integer>();
			schoolPreference.add(filler);
			for(int j = 0; j < problem.getHighSchoolPreference().get(0).size(); j++){
				schoolPreference.get(i).add(problem.getHighSchoolPreference().get(i).get(j));
			}
		}
		
		ArrayList<ArrayList<Integer>> studentPreference = new ArrayList<ArrayList<Integer>>();
		for(int i = 0; i < problem.getStudentPreference().size(); i++){
			ArrayList<Integer> filler = new ArrayList<Integer>();
			studentPreference.add(filler);
			for(int j = 0; j < problem.getStudentPreference().get(0).size(); j++){
				studentPreference.get(i).add(problem.getStudentPreference().get(i).get(j));
			}
		}

		ArrayList<ArrayList<Integer>> rankMap = makeRankMap(problem.getStudentPreference());
		int student = 0;
		while(((matchingArray.get(student) == -1) && (numOptions(student, studentPreference) > 0)) || (numMatched(matchingArray, problem) < problem.totalHighSchoolSpots())){//run through every student O(n)
			int highSchool  = validHighSchool(studentPreference.get(student));
			matchingArray.set(student, highSchool);
			if(problem.getHighSchoolSpots().get(highSchool) < numMatchedTo(matchingArray, highSchool)){
				int removeIndex = leastPreferredIndex(problem, highSchool, matchingArray);
				int leastDesiredStudent = schoolPreference.get(highSchool).get(removeIndex);
				matchingArray.set(leastDesiredStudent, -1);
			}
			if(problem.getHighSchoolSpots().get(highSchool) == numMatchedTo(matchingArray, highSchool)){
				int leastIndex = leastPreferredIndex(problem, highSchool, matchingArray);
				int removeIndex = leastIndex + 1;
				while(removeIndex < schoolPreference.get(highSchool).size()){
					
					int removedStudent = schoolPreference.get(highSchool).get(removeIndex);
					schoolPreference.get(highSchool).set(removeIndex, -1);
					if(removedStudent != -1){
						int schoolIndexRemovedStudent = rankMap.get(highSchool).get(removedStudent);
						studentPreference.get(removedStudent).set(schoolIndexRemovedStudent, -1);
					}
					removeIndex++;
				}
			}

			
		//check matching, use a method
		//match with first hospital
		//check if that hospital is oversubscribed and remove least desired resident if yes
		//check if hospital is full, find the least desired resident
		//delete thing
			if(student < problem.getStudentCount() - 1){
				student++;
			}
			else if(student == problem.getStudentCount() - 1){
				student = 0;
			}
		
		}		
		problem.setStudentMatching(matchingArray);
		return problem;
	}
    /**
     * Determines a solution to the stable matching problem from the given input set. Study the
     * project description to understand the variables which represent the input to your solution.
     *
     * @return A stable Matching.
     */
    @Override
	
	public Matching stableMatchingGaleShapley_highschooloptimal(Matching problem){
		//set matching students to 0 and fill it
		ArrayList<Integer> matchingArray = new ArrayList<Integer>();
		for(int i = 0; i < problem.getStudentCount(); i++){
			matchingArray.add(-1);
		}

		//create the temporary preference lists from the school and the students
		ArrayList<ArrayList<Integer>> schoolPreference = new ArrayList<ArrayList<Integer>>();
		for(int i = 0; i < problem.getHighSchoolPreference().size(); i++){
			ArrayList<Integer> filler = new ArrayList<Integer>();
			schoolPreference.add(filler);
			for(int j = 0; j < problem.getHighSchoolPreference().get(0).size(); j++){
				schoolPreference.get(i).add(problem.getHighSchoolPreference().get(i).get(j));
			}
		}
		
    	ArrayList<ArrayList<Integer>> studentPreference = new ArrayList<ArrayList<Integer>>();
		for(int i = 0; i < problem.getStudentPreference().size(); i++){
			ArrayList<Integer> filler = new ArrayList<Integer>();
			studentPreference.add(filler);
			for(int j = 0; j < problem.getStudentPreference().get(0).size(); j++){
				studentPreference.get(i).add(problem.getStudentPreference().get(i).get(j));
			}
		}
		//create the rank map with the getHighSchoolPreferences
		ArrayList<ArrayList<Integer>> rankMap = makeRankMap(problem.getHighSchoolPreference());


		//while loop that cycles through highSchools
		int highSchool = 0;
		while(numMatched(matchingArray, problem) < problem.totalHighSchoolSpots()){
			
		//create undersubscribed method and place it as the condition of a while loop to check if current hS is under-subscribed
			
			while(undersubscribed(highSchool, problem, matchingArray)){//O(m)
		//find first student that is not assigned using method validStudent (takes the rankMap)
				int student = validStudent(highSchool, schoolPreference.get(highSchool), matchingArray);
				if(student == -1){
					break;
				}
		//if getStudentMatching.get(student) != -1, set it equal to negative one
				if(matchingArray.get(student) != -1){
					matchingArray.set(student, -1);
				}
		//set getStudentMatching.get(student) to high school
				matchingArray.set(student, highSchool);
		//create delete method that deletes all below and deletes from
				int indexOfHighSchool = studentPreference.get(student).indexOf(highSchool);
				int removeIndex = indexOfHighSchool + 1;
				while(removeIndex < studentPreference.get(student).size()){
					int removedHighSchool = studentPreference.get(student).get(removeIndex);
					studentPreference.get(student).set(removeIndex, -1);
					if(removedHighSchool != -1){
						int studentIndexRemovedHS = rankMap.get(student).get(removedHighSchool);
						schoolPreference.get(removedHighSchool).set(studentIndexRemovedHS, -1);
					}
					removeIndex++;
				}	
			} 
			highSchool++;
			if(highSchool >= problem.getHighSchoolCount()){
				highSchool = 0;
			}
			problem.setStudentMatching(matchingArray);
		}
		return problem;
	}
	
    
    public static ArrayList<Integer> studentsAtSchool(ArrayList<Integer> matchingArray, int hs){
    	ArrayList<Integer> returnList = new ArrayList<Integer>();
    	for(int i = 0; i < matchingArray.size(); i++) {
    		if(matchingArray.get(i) == hs) {
    			returnList.add(i);
    		}
    	}
    	return returnList;
    }
    
    public static int numMatched(ArrayList<Integer> matchingArray, Matching problem) {//returns the number of matched students
    	
    	int count = 0;
    	for(int i = 0; i < matchingArray.size(); i++) {
    		if(matchingArray.get(i) == -1) {
    			count++;
    		}
    	}
    	int returnInt = problem.getStudentCount() - count;
    	return returnInt;
    }
    
    public static int numMatchedTo(ArrayList<Integer> matchingArray, int index) {//returns number matched to specific school
    	int returnInt = 0;
    	
    	for(int i = 0; i < matchingArray.size(); i++) {
    		if(matchingArray.get(i) == index) {
    			returnInt++;
    		}
    	}
    	
    	return returnInt;
    }
    
    public static int leastPreferredIndex(Matching problem, int index, ArrayList<Integer> matchingArray) {//returns the index of the matched student least preferred by a school
    	ArrayList<Integer> specificPreference = problem.getHighSchoolPreference().get(index);
    	int indexLeastPreferred = -1;
    	for(int i = 0; i < matchingArray.size();i++) {
    		if(matchingArray.get(i) == index) {
    			if(specificPreference.indexOf(i) > indexLeastPreferred) {
    				indexLeastPreferred = specificPreference.indexOf(i);
    			}
    		}
    	}
    	
    	return indexLeastPreferred;
    }

	
	public static ArrayList<ArrayList<Integer>> makeRankMap(ArrayList<ArrayList<Integer>> preferenceArrays){
		ArrayList<ArrayList<Integer>> rankMap = new ArrayList<ArrayList<Integer>>();
		
		
		for(int i = 0; i < preferenceArrays.get(0).size(); i++){
			ArrayList<Integer> filler = new ArrayList<Integer>();
			rankMap.add(filler);
			for(int j = 0; j < preferenceArrays.size(); j++){
				rankMap.get(i).add(-1);
			}
		}
		for(int i = 0; i < rankMap.get(0).size(); i++){//makes rankMap an appropriate size and fills them with invalid inputs
			for(int j = 0; j < rankMap.size(); j++){
				int student = preferenceArrays.get(i).get(j);//student is the jth ranked student in the ith high school
				int schoolRow = i;//gives the school row in rankMap to place the index
				int studentRank = j;//gives the rank of the student in the ith high school
				rankMap.get(student).set(schoolRow, studentRank);

			}
		}

		return rankMap;
	}

	public static boolean undersubscribed(int highSchool, Matching problem, ArrayList<Integer> matchingArray){
		int numSpots = problem.getHighSchoolSpots().get(highSchool);
		int numAtSchool = studentsAtSchool(matchingArray, highSchool).size();
		if(numSpots > numAtSchool){
			return true;
		}
		else{
			return false;
		}
	}

	public static int validStudent(int highSchool, ArrayList<Integer> highSchoolPreference, ArrayList<Integer> matchingArray){
		int student = -1;
		for(int i = 0; i < highSchoolPreference.size(); i++){
			student = highSchoolPreference.get(i);
			if((student != -1) && (matchingArray.get(student) != highSchool )){
				i = highSchoolPreference.size();
			}
		}
		return student;
	}
	
	public static int numOptions(int student, ArrayList<ArrayList<Integer>> studentPreferenceList){
		int returnNum = 0;
		for(int i = 0; i < studentPreferenceList.get(student).size(); i++){
			if(studentPreferenceList.get(student).get(i) != -1){
				return 1;
			}
		}
		return returnNum;
	}

	public static int validHighSchool(ArrayList<Integer> studentPreference){
		int school = -1;
		for(int i = 0; i < studentPreference.size(); i++){
			school = studentPreference.get(i);
			if(school != -1){
				i = studentPreference.size();
			}
		}
		return school;
	}
}