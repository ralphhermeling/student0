To test manually whether coordinator re-assigns task to other workers insert printf and sleep statements
before workers begin executing map tasks. Specifically, you will want to add a sleep statement right after the worker calls get_task_1 on line 23 of worker/worker.c in the starter code so that the worker crashes after it receives a task but before it executes it.
You can then manually crash a worker before it completes a task by pressing Ctrl-C.

We need to have a queue of job ids -> Queue
We need to have monotonically increasing job IDs -> Global variable next_job_id
We need to have efficient job lookup by ID -> HashTable

The workers pull map tasks and reduce tasks and reduce tasks can only start after map tasks have finished

The queue should work FIFO. The coordinator is responsible for prioritizing the completion of jobs that are submitted earlier.

The coordinator should keep track of whether job has completed mapping step and if so start scheduling reduce tasks. 
We can do this by keeping track of n_map_tasks and n_map_tasks_completed for a given job.

Once n_map_tasks equals n_map_tasks_completed schedule reduce tasks
Once n_reduce_tasks equals n_reduce_tasks_completed set status to completed

Use a min heap with job_id as the identifier
Advantages of min-heap approach:
  - Strict job priority: Tasks with lower job IDs always come first
  - Efficient: O(log n) insertion and extraction
  - Handles your edge case: Even if job 2's reduce tasks become available first, job 1's reduce tasks will be prioritized once they're added
  - Natural handling of failed task re-execution: A failed task keeps its original job ID priority

  Algorithm:
  1. Add all map tasks to heap immediately when job is submitted
  2. Add reduce tasks to heap only when all map tasks for that job complete
  3. Workers always get the task with minimum job_id from heap
  4. Failed tasks are re-inserted with their original job_id

  This gives you exactly the behavior you want: strict job priority while maximizing worker utilization.
  The heap ensures that even if later jobs complete their map phase first, earlier
   jobs' tasks will always be prioritized.
