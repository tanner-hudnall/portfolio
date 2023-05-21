//UT-EID=tch2368

import java.util.*;
import java.util.concurrent.*;

public class ForkJoinPSort {
    /* Notes:
     * The input array (A) is also the output array,
     * The range to be sorted extends from index begin, inclusive, to index end, exclusive,
     * Sort in increasing order when increasing=true, and decreasing order when increasing=false,
     */
    public static void parallelSort(int[] A, int begin, int end, boolean increasing) {
        ForkJoinPool commonPool = ForkJoinPool.commonPool();
        parallelSortHelper(A, begin, end, commonPool);
  
        commonPool.shutdown();   
        try {      //Change
            commonPool.awaitTermination(200, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if(!increasing){
            reverse(A);
        }
        
    }
    public static void parallelSortHelper(int[] A, int begin, int end, ForkJoinPool commonPool){
        if(A.length == 0){
            return;
        }
        else if((end - begin) <= 16){
            sorter path = new sorter(A, begin, end - 1, false);
            commonPool.submit(path);      //Change
        }
        else if(end > begin){
            sorter path = new sorter(A, begin, end - 1, true);
            int pivotIndex = (int) commonPool.invoke(path);      //Change
            parallelSortHelper(A, begin, pivotIndex, commonPool); 
            parallelSortHelper(A, pivotIndex + 1, end, commonPool);
        }
    }

    public static int quickSort(int[] arr, int start, int end){
        int pivot = arr[end];
        int i = start - 1;
        for(int j = start; j < end; j++){
            if(arr[j] < pivot){
                i++;
                swap(i, j, arr);
            }
        }
        swap(i+1, end, arr); //make note because this can fuck stuff up
        return i+1;
    }
    
     public static void insertSort(int[] arr, int start, int end){
        for(int i = start + 1; i <= end; i++){
            int j = i;
            while(j > 0 && arr[j-1] > arr[j]){
                swap(j,j-1,arr);
                j--;
            }
        }   
    }
    
    public static void swap(int idx1, int idx2, int[] arr){
        int temp = arr[idx1];
        arr[idx1] = arr[idx2];
        arr[idx2] = temp;
    }

    public static void reverse(int[] arr){
        if(arr.length % 2 == 0){
            for(int i = 0; i < arr.length / 2; i++){
                int temp = arr[i];
                arr[i] = arr[arr.length - 1 - i];
                arr[arr.length - 1 - i] = temp;
            }
        }
        else{
            for(int i = 0; i <= arr.length / 2; i++){
                int temp = arr[i];
                arr[i] = arr[arr.length - 1 - i];
                arr[arr.length - 1 - i] = temp;
            }
        }
    }
    public static int pivotIndexFinder(int[] arr, int start, int end){ // effectively just a quicksort that reads rather than writes (no swaps)
        int pivot = arr[end];
        int i = start - 1;
        for(int j = start; j < end; j++){
            if(arr[j] < pivot){
                i++;
            }
        }
        return i+1;
    }

    private static class sorter extends RecursiveTask<Integer> {
        int[] arr;
        int start;
        int end;
        boolean qS;

        public sorter(int[] arrayToSort, int startSortSection, int endSortSection, boolean isQuickSort){
            this.arr = arrayToSort;
            start = startSortSection;
            end = endSortSection;
            qS = isQuickSort;
        }

        public Integer compute(){
            if(qS){
                int res = quickSort(arr, start, end);
                //System.out.println("quickSort result: "+ Arrays.toString(arr));
                return res;
            }
            else{
                insertSort(arr, start, end);
                return -1;
                //System.out.println("insertSort result: "+ Arrays.toString(arr));
            }
        }
    }
}