void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  //enum intr_level old_level = intr_disable();

  struct thread *t_holder = lock->holder;

  if(t_holder != NULL){
    //Como ya hay un thread que tiene el lock, hay que esperar
    thread_current()->waitingLock = lock;

    //list_push_front(&t_holder->holdingLocks, &lock->lock_elemen);
    
    //if(t_holder->priority < thread_current()->priority)
    //Como el current thread no es el holder, se intenta hacer donación al holder
      //priorityDonation(t_holder, thread_current()->priority);
  }
   else {
    thread_current()->waitingLock = NULL;
   } 
    //Una vez el lock holder sea nulo, entonces el current thread hace arquire del lock
    sema_down (&lock->semaphore);
    lock->holder = thread_current();
    list_push_front(&thread_current()->holdingLocks, &lock->lock_elemen);
  
    //intr_set_level(old_level);
}


void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  enum intr_level old_level = intr_disable();

  priorityDonationInverse(thread_current(),lock);

  lock->holder = NULL;
  sema_up (&lock->semaphore);
  intr_set_level(old_level);
}
