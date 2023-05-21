public class MonitorCyclicBarrier implements CyclicBarrier {
    private int parties;
    private int currentParties;
    private boolean isActive;

    public MonitorCyclicBarrier(int parties) {
        this.parties = parties;
        this.currentParties = 0;
        this.isActive = true;
    }

    @Override
    public void activate() throws InterruptedException {
        this.isActive = true;
        this.currentParties = 0;
    }

    @Override
    public void deactivate() throws InterruptedException {
        this.isActive = false;
        synchronized (this) {
            this.notifyAll();
        }
    }

    @Override
    public int await() throws InterruptedException {
        int index = -1;
        if (isActive) {
            synchronized (this) {
                currentParties++;
                index = currentParties - 1;
                if (currentParties < parties) {
                    this.wait();
                } else {
                    this.notifyAll();
                    currentParties = 0;
                }
            }
        }
        return index;
    }
}