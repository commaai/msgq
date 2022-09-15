package messaging;

import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.TimeUnit;

public class Event {
    Lock lock = new ReentrantLock();
    Condition cond = lock.newCondition();
    boolean flag;
    public void doWait() throws InterruptedException {
        lock.lock();
        try {
            while (!flag) {
                cond.await();
            }
        } finally {
            lock.unlock();
        }
    }
    public void doWait(float seconds) throws InterruptedException {
        lock.lock();
        try {
            while (!flag){
                cond.await((int)(seconds * 1000), TimeUnit.MILLISECONDS);
            }
        } finally {
            lock.unlock();
        }
    }
    public boolean isSet() {
        lock.lock();
        try {
            return flag;
        } finally {
            lock.unlock();
        }
    }
    public void set() {
        lock.lock();
        try {
            flag = true;
            cond.signalAll();
        } finally {
            lock.unlock();
        }
    }
    public void clear() {
        lock.lock();
        try {
            flag = false;
            cond.signalAll();
        } finally {
            lock.unlock();
        }
    }
}

