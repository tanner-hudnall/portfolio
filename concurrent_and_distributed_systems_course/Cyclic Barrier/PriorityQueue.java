import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

public class PriorityQueue {
   private static class Node {
      private String name;
      private int priority;
      private Node next;

      public Node(String name, int priority) {
         this.name = name;
         this.priority = priority;
      }
   }

   private Node head;
   private int size;
   private int capacity;
   private ReentrantLock lock = new ReentrantLock();
   private Condition notFull = lock.newCondition();
   private Condition notEmpty = lock.newCondition();

   public PriorityQueue(int capacity) {
      this.capacity = capacity;
   }

   public int add(String name, int priority) throws InterruptedException {
      lock.lock();
      try {
         while (size == capacity) {
            notFull.await();
         }
         Node newNode = new Node(name, priority);
         if (head == null || head.priority < priority) {
            newNode.next = head;
            head = newNode;
         } else {
            Node current = head;
            int index = 0;
            while (current.next != null && current.next.priority >= priority) {
               current = current.next;
               index++;
            }
            newNode.next = current.next;
            current.next = newNode;
            index++;
            size++;
            notEmpty.signalAll();
            return index;
         }
         size++;
         notEmpty.signalAll();
         return 0;
      } finally {
         lock.unlock();
      }
   }

   public int search(String name) {
      lock.lock();
      try {
         Node current = head;
         int index = 0;
         while (current != null) {
            if (current.name.equals(name)) {
               return index;
            }
            current = current.next;
            index++;
         }
         return -1;
      } finally {
         lock.unlock();
      }
   }

   public String getFirst() throws InterruptedException {
      lock.lock();
      try {
         while (head == null) {
            notEmpty.await();
         }
         Node removed = head;
         head = head.next;
         size--;
         notFull.signalAll();
         return removed.name;
      } finally {
         lock.unlock();
      }
   }
}