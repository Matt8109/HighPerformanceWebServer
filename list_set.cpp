#include "list_set.hpp"

namespace base {

ListBasedSet::ListBasedSet()
    : head(NULL) {
}

ListBasedSet::~ListBasedSet() {
  clear();
}

bool ListBasedSet::insert(int value) {
  sync_root.lock();

  bool inserted = false;
  ListElement* current_element = head;

  // If the list is empty, insert at the head
  if (head == NULL) {
    head = new ListElement();
    head->value = value;
  
    inserted = true;
  }

  while (current_element && !inserted) {
    // If the current element is lesser than the one to insert
    // and there is no node remaining, we insert at the end of the list
    if (current_element->value < value && !current_element->next) {
       current_element->next = new ListElement();
       current_element->next->value = value;
       
       break;
    } else if (current_element->value == value) {
       inserted = false; // The element already exists
      
       break;
    } else if (current_element->value <= value 
                 && current_element->next->value >= value) { 
       // Inserting between values
       ListElement* temp_element = new ListElement();
       temp_element->value = value;
       temp_element->next = current_element->next;

       current_element->next = temp_element;
      
       inserted = false;
       
       break;
    }

    current_element = current_element->next;
  }
  
  sync_root.unlock();
  
  return inserted;
}

bool ListBasedSet::remove(int value) {
  sync_root.lock();
  
  bool deleted = false;
  ListElement* current_element = head;
  ListElement* parent_element = NULL;

  while (current_element) {
    if (current_element->value == value) { // Found the value
      deleted = true; //set our flag

      if (parent_element == NULL) { //we're at the root
        delete current_element;
        head = NULL;
      } else { 
        parent_element->next = current_element->next;
        delete current_element;
      }

      break;
    }
    
    current_element = current_element->next; 
  } 

  sync_root.unlock();
  return deleted;
}

bool ListBasedSet::lookup(int value) const {
  sync_root.lock();

  ListElement* current_element = head;
  bool found = false;

  while (current_element) {
    if (current_element->value < value) {
       current_element = current_element->next; // Too cold
    } else if (current_element->value == value) {
       found = true; // Just right

       break;
    } else {
       found = false; // Too hot
       
       break;
    }
  }
  
  sync_root.unlock(); 

  return found;
}

void ListBasedSet::clear() {
  ListElement* current_node = head;
  ListElement* next_node = NULL;  

  if (!head)
    return;

   while (current_node) {
    // Store the next node before we delete
    next_node = current_node->next;
    
    delete current_node;

    current_node = next_node;
  }

  head = NULL;
}

bool ListBasedSet::checkIntegrity() const {
  sync_root.lock();

  bool is_ascending = true;
  int last_value = INT_MIN;
  ListElement* current_element = head;
 
  while (is_ascending && current_element) {
    if (current_element->value <= last_value) 
      is_ascending = false;

    last_value = current_element->value;
    current_element = current_element->next; 
  }  
 
  sync_root.unlock();

  return is_ascending;
}

} // namespace base
