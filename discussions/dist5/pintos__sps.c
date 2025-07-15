struct thread {
int priority;
struct list_elem elem;
}

struct list ready_list;

void thread_unblock (struct thread *t) {
 ASSERT(is_thread(t));

 enum intr_level old_level;
 old_level = intr_disable();
 ASSERT(t->status == THREAD_BLOCKED);

 if (t->priority == 1) {
 list_push_front(ready_list, t->elem);
 } else {
 list_push_back(ready_list, t->elem);
 }

 t->status = THREAD_READY;
 intr_set_level(old_level);
}
