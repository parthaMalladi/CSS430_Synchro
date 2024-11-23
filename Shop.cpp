#include "Shop.h"

void Shop::init() 
{
   pthread_mutex_init(&mutex_, NULL);
   for (int i = 0; i < max_barbers_; i++) {
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

void Shop::printCustomer(int person, string message)
{  
   cout << "customer[" << person << "]: " << message << endl;
}

void Shop::printBarber(int person, string message)
{  

   cout << "barber[" << person << "]: " << message << endl;
}

int Shop::get_cust_drops() const
{
   return cust_drops_;
}

int Shop::visitShop(int id) 
{
   pthread_mutex_lock(&mutex_);

   // leave the shop if there are no waiting chairs and no barbers available
   if (waiting_chairs_.size() == max_waiting_cust_ && available_barbers_.empty()) {
      printCustomer(id, "leaves the shop because of no available waiting chairs.");
      ++cust_drops_;
      pthread_mutex_unlock(&mutex_);
      return -1;
   }

   // create a customer and put them into the map
   customers[id] = Customer();
   customers[id].myId = id;
   int barber = -1;

   if (available_barbers_.empty()) {
      waiting_chairs_.push(id);
      printCustomer(id, "takes a waiting chair. # waiting seats available = " + int2string(max_waiting_cust_ - waiting_chairs_.size()));

      while (customers[id].myBarber == -1) {
         pthread_cond_wait(&customers[id].cond_customers_waiting_, &mutex_);
      }
      barber = customers[id].myBarber;
   } else {
      barber = available_barbers_.front();
      available_barbers_.pop();
      customers[id].myBarber = barber;
      customer_in_chair_[barber] = id;
      in_service_[barber] = true;
   }

   printCustomer(id, "moves to the service chair. # waiting seats available = " + int2string(max_waiting_cust_ - waiting_chairs_.size()));
   pthread_cond_signal(&cond_barber_sleeping_[barber]);
   pthread_mutex_unlock(&mutex_);
   return barber;
}

void Shop::leaveShop(int id, int barber_id) 
{
   pthread_mutex_lock(&mutex_);
   printCustomer(id, "wait for the hair-cut to be done by " + int2string(barber_id));

   while (customers[id].myBarber != -1) {
      pthread_cond_wait(&customers[id].cond_customer_served_, &mutex_);
   }

   // Pay the barber and signal barber appropriately
   money_paid_[barber_id] = true;
   pthread_cond_signal(&cond_barber_paid_[barber_id]);
   printCustomer( id, "says good-bye to the barber." );
   pthread_mutex_unlock(&mutex_);
}

void Shop::helloCustomer(int barber_id) 
{
   pthread_mutex_lock(&mutex_);

   if (waiting_chairs_.empty() && customer_in_chair_[barber_id] == 0) {
      printBarber(barber_id, "sleeps because of no customers");
      available_barbers_.push(barber_id);

      while (customer_in_chair_[barber_id] == 0) {
         pthread_cond_wait(&cond_barber_sleeping_[barber_id], &mutex_);
      }
   }

   while (customer_in_chair_[barber_id] == 0) {
      pthread_cond_wait(&cond_barber_sleeping_[barber_id], &mutex_);
   }
   
   printBarber(barber_id, "starts a hair-cut service for " + int2string(customer_in_chair_[barber_id]));
   pthread_mutex_unlock( &mutex_ );
}

void Shop::byeCustomer(int barber_id) 
{
   pthread_mutex_lock(&mutex_);
   int client = customer_in_chair_[barber_id];

   in_service_[barber_id] = false;
   printBarber(barber_id, "says he's done with a hair-cut service for " + int2string(customer_in_chair_[barber_id]));
   money_paid_[barber_id] = false;
   customers[client].myBarber = -1;
   pthread_cond_signal(&customers[client].cond_customer_served_);

   while (money_paid_[barber_id] == false) {
      pthread_cond_wait(&cond_barber_paid_[barber_id], &mutex_);
   }

   customer_in_chair_[barber_id] = 0;
   customers.erase(client);

   printBarber(barber_id, "calls in another customer");
   if (!waiting_chairs_.empty()) {
      int newClient = waiting_chairs_.front();
      waiting_chairs_.pop();

      customers[newClient] = Customer();
      customers[newClient].myId = newClient;
      customers[newClient].myBarber = barber_id;
      customer_in_chair_[barber_id] = newClient;
      in_service_[barber_id] = true;
      
      pthread_cond_signal(&customers[newClient].cond_customers_waiting_);
   }

   pthread_mutex_unlock( &mutex_ );
}
