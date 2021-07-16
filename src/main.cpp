#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <random>
#include <atomic>

#include "../include/header.h"



std::ostream& operator<<(std::ostream& out, Dish object)
{
    switch (object)
    {
        case Dish::PIZZA : out << "pizza";
            break;
        case Dish::SALAD : out << "salad";
            break;
        case Dish::SOUP : out << "soup";
            break;
        case Dish::STEAK : out << "steak";
            break;
        case Dish::SUSHI : out << "sushi";
            break;
        default:
            break;
    }

    return out;
}



void output(const std::string& str, Dish dish, std::mutex& mutexCout)
{
    std::lock_guard<std::mutex> lg(mutexCout);

    std::cout << str << dish << '\n';
}



//Официант принимает и оформляет заказы (заказы передаются на кухню)

void waiterWork(Waiter* waiter, Queues* queues, std::atomic<bool>& workEnd,
                std::mutex& mutexOrderKitchen, std::condition_variable& condVarOrderKitchen, std::mutex& mutexCout)
{
    Dish dish;

    while (!workEnd.load())
    {
        //Официант определяет блюдо
        dish = waiter->dishDifining();

        //Официант оформляет заказ
        std::this_thread::sleep_for(waiter->orderMaking());

        //Официант добавляет заказ в очередь
        {
            std::lock_guard<std::mutex> lg(mutexOrderKitchen);

            queues->addOrder(dish);

            output("The waiter placed an order for -> ", dish, mutexCout);
        }

        //Официант информирует кухню о появлении нового заказа
        condVarOrderKitchen.notify_one();
    }
}



//Кухня ожидает заказы и готовит блюда (о готовых сообщается курьеру)

void kitchenWork(Kitchen* kitchen, Queues* queues, std::atomic<bool>& workEnd,
                 std::mutex& mutexOrderKitchen, std::condition_variable& condVarOrderKitchen,
                 std::mutex& mutexKitchenCourier, std::condition_variable& condVarKitchenCourier, std::mutex& mutexCout)
{
    Dish dish;

    while (!workEnd.load())
    {
        //Кухня ожидает заказ и удаляет его из очереди заказов
        {
            std::unique_lock<std::mutex> ul(mutexOrderKitchen);

            condVarOrderKitchen.wait(ul, [&] { return !queues->emptyOrder(); });

            dish = queues->delOrder();
        }

        //Кухня готовит блюдо
        std::this_thread::sleep_for(kitchen->dishPrepares());

        //Кухня добавляет готовое блюдо в очередь готовых
        {
            std::lock_guard<std::mutex> lg(mutexKitchenCourier);

            queues->addDish(dish);

            output("The kitchen has prepared -> ", dish, mutexCout);
        }

        //Кухня информирует курьера о появлении нового блюда
        condVarKitchenCourier.notify_one();
    }
}



//Курьер ожидает готовые блюда и доставляет их клиентам

void courierWork(Courier* courier, Queues* queues, int& deliveryCounter, std::atomic<bool>& workEnd,
                 std::mutex& mutexKitchenCourier, std::condition_variable& condVarKitchenCourier,
                 std::mutex& mutexWorkEnd, std::condition_variable& condVarWorkEnd, std::mutex& mutexCout)
{
    while (!workEnd.load())
    {
        //Курьер забирает готовые блюда, очищает очередь
        {
            std::unique_lock<std::mutex> ul(mutexKitchenCourier);

            condVarKitchenCourier.wait(ul, [&] { return !queues->emptyDish(); });

            while (!queues->emptyDish())
            {
                output("The courier took -> ", queues->getDish(), mutexCout);
                queues->delDish();
            }
        }

        //Увеличение счётчика общих доставок
        {
            std::lock_guard<std::mutex> lg(mutexWorkEnd);

            ++deliveryCounter;
        }

        //Курьер информирует главного, не пора ли закончить работу
        condVarWorkEnd.notify_one();

        //Курьер отправился на доставку
        std::this_thread::sleep_for(courier->getInterval());
    }
}



int main()
{
    //Диапазон времени оформления заказа
    const int orderMin{ 5 };
    const int orderMax{ 10 };

    //Диапазон времени приготовления блюда
    const int prepMin{ 5 };
    const int prepMax{ 15 };

    //Интервал времени, через который курьер забирает готовые блюда с кухни
    const std::chrono::seconds interval{ 30 };

    //Счётчик общих доставок
    int deliveryCounter{};

    //Максимальное количество общих доставок (партий блюд)
    const int numberOfDeliveries{ 10 };

    //Две очереди: заказы и готовые блюда
    Queues* queues = new Queues;

    //Флаг сигнализации окончания работы
    std::atomic<bool> workEnd{};

    std::mutex mutexCout;

    std::mutex mutexOrderKitchen;
    std::mutex mutexKitchenCourier;
    std::mutex mutexWorkEnd;

    std::condition_variable condVarOrderKitchen;
    std::condition_variable condVarKitchenCourier;
    std::condition_variable condVarWorkEnd;

    //Онлайн-ресторан начал работу

    std::cout << "Restaurant has started working\n\n";

    Waiter* waiter = new Waiter(orderMin, orderMax);
    std::thread waiterThread(waiterWork, waiter, queues, std::ref(workEnd),
                             std::ref(mutexOrderKitchen), std::ref(condVarOrderKitchen), std::ref(mutexCout));

    Kitchen* kitchen = new Kitchen(prepMin, prepMax);
    std::thread kitchenThread(kitchenWork, kitchen, queues, std::ref(workEnd),
                              std::ref(mutexOrderKitchen), std::ref(condVarOrderKitchen),
                              std::ref(mutexKitchenCourier), std::ref(condVarKitchenCourier), std::ref(mutexCout));

    Courier* courier = new Courier(interval);
    std::thread courierThread(courierWork, courier, queues, std::ref(deliveryCounter), std::ref(workEnd),
                              std::ref(mutexKitchenCourier), std::ref(condVarKitchenCourier),
                              std::ref(mutexWorkEnd), std::ref(condVarWorkEnd), std::ref(mutexCout));

    //Главный решает, когда пора закончить работу
    {
        std::unique_lock<std::mutex> ul(mutexWorkEnd);

        condVarWorkEnd.wait(ul, [&] { return deliveryCounter == numberOfDeliveries; });
    }

    //Заканчиваем! Сегодня потрудились хорошо
    workEnd.store(true);

    waiterThread.join();
    kitchenThread.join();
    courierThread.join();

    std::cout << '\n' << numberOfDeliveries << " deliveries completed successfully";
}