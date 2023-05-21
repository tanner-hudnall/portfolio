//UT-EID=tch2368

import java.util.*;
import java.util.concurrent.*;

public class PMerge {
    /* Notes:
     * Arrays A and B are sorted in the ascending order
     * These arrays may have different sizes.
     * Array C is the merged array sorted in the descending order
     */
    public static void parallelMerge(int[] A, int[] B, int[] C, int numThreads) {
        ExecutorService exec = Executors.newFixedThreadPool(numThreads); //ExecutorService starts fixed thread pool
        for(int i = 0; i < A.length; i++){ //for loops go in parallel merge 
            Future future = exec.submit(new placer(A[i], B, true, C, i)); //Executor.submit(new __class that calculates placement )

        }
        for(int i = 0; i < B.length; i++){
            Future future = exec.submit(new placer(B[i], A, false, C, i));
        }
        exec.shutdown();
        try {
            exec.awaitTermination(60, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        //executor.shutdown, executor.awaitTermination
    }

    public static int indexFinder(int comp, int[] arr, boolean order){//if order is true then its upper bound
        int start = 0;
        int end = arr.length - 1;
        if((comp == arr[end] && order) || comp > arr[end]){
            return end + 1;
        }
        else if(comp == arr[end] && !order){
            return end;
        }
        while(start < end){
            int mid = (end + start) / 2;
            if(comp > arr[mid]){
                start = mid + 1;
            }
            else if(comp == arr[mid] && order){
                return mid + 1;
            }
            else if(comp == arr[mid] && !order){
                return mid;
            }
            else{
                end = mid;
            }
        }
        return start;

    }

    private static class placer implements Runnable {
        int comp;
        int[] arr, res;
        boolean uB;
        int index;

        public placer(int comparator, int[] arrayToCompare, boolean upperBound, int[] result, int idx_at_home){
            comp = comparator;
            arr = arrayToCompare;
            uB = upperBound;
            res = result;
            index = idx_at_home;
        }
        public void run(){ //idx and res goes in run
            int reverseIdx =  indexFinder(comp, arr, uB) + index;
            int idx = res.length - 1 - reverseIdx;
            res[idx] = comp;
        }

    }
    
}
