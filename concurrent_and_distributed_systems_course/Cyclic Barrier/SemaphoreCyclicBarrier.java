import java.util.concurrent.Semaphore;

import javax.naming.PartialResultException;

public class SemaphoreCyclicBarrier implements CyclicBarrier {
    private int parties;
    private int currentParties;
    private boolean isActive;
    private Semaphore semaphore;

    public SemaphoreCyclicBarrier(int parties) {
        this.parties = parties;
        this.currentParties = 0;
        this.isActive = true;
        this.semaphore = new Semaphore(parties);
    }

    @Override
    public void activate() throws InterruptedException {
        this.isActive = true;
        this.currentParties = 0;
        this.semaphore = new Semaphore(parties);
    }

    @Override
    public void deactivate() throws InterruptedException {
        this.isActive = false;
        this.semaphore.release();
    }

    @Override
    public int await() throws InterruptedException {
        int index = -1;
        if (isActive) {
            synchronized (this) {
                currentParties++;
                index = currentParties - 1;
                if(currentParties == parties) {
                    semaphore.release();
                    currentParties = 0;
                }
            }
            semaphore.acquire();
        }
        return index;
    }
}



//when releasing, decrement the first index, increment the second
//if first index = 0, then switch the values