import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

public class FairUnifanBathroom {
    private ReentrantLock lock;
    private Condition utCondition;
    private Condition ouCondition;
    private int countUT;
    private int countOU;
    private int ticketNumber;

    public FairUnifanBathroom() {
        lock = new ReentrantLock();
        utCondition = lock.newCondition();
        ouCondition = lock.newCondition();
        countUT = 0;
        countOU = 0;
        ticketNumber = 0;
    }

    public void enterBathroomUT() {
        lock.lock();
        try {
            int myTicketNumber = ticketNumber++;
            while ((countOU > 0 || countUT >= 7) && myTicketNumber != 0) {
                utCondition.await();
            }
            countUT++;
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            lock.unlock();
        }
    }

    public void enterBathroomOU() {
        lock.lock();
        try {
            int myTicketNumber = ticketNumber++;
            while ((countUT > 0 || countOU >= 7) && myTicketNumber != 0) {
                ouCondition.await();
            }
            countOU++;
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            lock.unlock();
        }
    }

    public void leaveBathroomUT() {
        lock.lock();
        try {
            countUT--;
            if (countUT == 0) {
                ouCondition.signalAll();
            } else {
                utCondition.signal();
            }
        } finally {
            lock.unlock();
        }
    }

    public void leaveBathroomOU() {
        lock.lock();
        try {
            countOU--;
            if (countOU == 0) {
                utCondition.signalAll();
            } else {
                ouCondition.signal();
            }
        } finally {
            lock.unlock();
        }
    }
}