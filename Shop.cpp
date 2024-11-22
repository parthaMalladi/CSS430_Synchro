
#include "Shop.h"

void Shop::init() 
{
   pthread_mutex_init(&mutex_, NULL);
   pthread_cond_init(&cond_customers_waiting_, NULL);

   for (int i = 0; i < max_barbers_; i++) {
      pthread_cond_init(&cond_customer_served_[i], NULL);
      pthread_cond_init(&cond_barber_paid_[i], NULL);
      pthread_cond_init(&cond_barber_sleeping_[i], NULL);
   }
}

string Shop::int2string(int i) 
{
   stringstream out;
   out << i;
   return out.str( );
}

void Shop::print(int person, string message)
{  
   if (person > 0) {
      cout << "customer[" << person << "]: " << message << endl;
   } else {
      cout << "barber[" << person << "]: " << message << endl;
   }
}

int Shop::get_cust_drops() const
{
   return cust_drops_;
}

int Shop::visitShop(int id) 
{
   pthread_mutex_lock(&mutex_);

   if (waiting_chairs_.size() == max_waiting_cust_) 
   {
      print(id, "leaves the shop because of no available waiting chairs.");
      ++cust_drops_;
      pthread_mutex_unlock(&mutex_);
      return -1;
   }

   // Customer takes a waiting chair
   waiting_chairs_.push(id);
   print(id, "takes a waiting chair. # waiting seats available = " + int2string(max_waiting_cust_ - waiting_chairs_.size()));

   // Wait until a barber becomes available
   while (available_barbers_.empty()) {
      pthread_cond_wait(&cond_customers_waiting_, &mutex_);
   }

   // Get an available barber's index
   int barber_id = available_barbers_.front();
   available_barbers_.pop();

   // Assign customer to this barber
   waiting_chairs_.pop();
   customer_in_chair_[barber_id] = id;
   in_service_[barber_id] = true;
   print(id, "moves to the service chair. # waiting seats available = " + int2string(max_waiting_cust_ - waiting_chairs_.size()));

   // Signal the barber incase sleeping
   pthread_cond_signal(&cond_barber_sleeping_[barber_id]);

   pthread_mutex_unlock(&mutex_);
   return barber_id;
}

void Shop::leaveShop(int id, int barber_id) 
{
   pthread_mutex_lock( &mutex_ );

   // Wait for service to be completed
   print(id, "wait for the hair-cut to be done by " + int2string(barber_id));
   while (in_service_[barber_id] == true)
   {
      pthread_cond_wait(&cond_customer_served_[barber_id], &mutex_);
   }
   
   // Pay the barber and signal barber appropriately
   money_paid_[barber_id] = true;
   pthread_cond_signal(&cond_barber_paid_[barber_id]);
   print( id, "says good-bye to the barber." );
   pthread_mutex_unlock(&mutex_);
}

void Shop::helloCustomer(int barber_id) 
{
   pthread_mutex_lock(&mutex_);
   
   // If no customers than barber can sleep
   while (waiting_chairs_.empty() && customer_in_chair_[barber_id] == 0) 
   {  
      print((barber_id * -1), "sleeps because of no customers");
      pthread_cond_wait(&cond_barber_sleeping_[barber_id], &mutex_);
   }

   print((barber_id * -1), "starts a hair-cut service for " + int2string( customer_in_chair_[barber_id] ) );
   pthread_mutex_unlock( &mutex_ );
}

void Shop::byeCustomer(int barber_id) 
{
   pthread_mutex_lock(&mutex_);

   // Hair Cut-Service is done so signal customer and wait for payment
   in_service_[barber_id] = false;
   print((barber_id * -1), "says he's done with a hair-cut service for " + int2string(customer_in_chair_[barber_id]));
   money_paid_[barber_id] = false;
   pthread_cond_signal(&cond_customer_served_[barber_id]);
   while (money_paid_[barber_id] == false)
   {
      pthread_cond_wait(&cond_barber_paid_[barber_id], &mutex_);
   }

   //Signal to customer to get next one
   customer_in_chair_[barber_id] = 0;
   available_barbers_.push(barber_id);
   print((barber_id * -1), "calls in another customer");
   pthread_cond_signal( &cond_customers_waiting_ );

   pthread_mutex_unlock( &mutex_ );  // unlock
}
