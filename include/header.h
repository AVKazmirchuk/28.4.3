#pragma once

#include <queue>
#include <iostream>
#include <chrono>
#include <random>



enum class Dish
{
    PIZZA,
    SOUP,
    STEAK,
    SALAD,
    SUSHI,
    MAX
};



class Queues
{
private:
    //Очередь заказов; официант добавляет, кухня удаляет
    std::queue<Dish> orders;

    //Очередь блюд; кухня добавляет, курьер удаляет
    std::queue<Dish> dishes;
public:
    void addOrder(Dish dish)
    {
        orders.push(dish);
    }

    Dish delOrder()
    {
        Dish temp = orders.front();
        orders.pop();
        return temp;
    }

    bool emptyOrder()
    {
        return orders.empty();
    }

    Dish getDish()
    {
        return dishes.front();
    }

    void addDish(Dish dish)
    {
        dishes.push(dish);
    }

    void delDish()
    {
        dishes.pop();
    }

    bool emptyDish()
    {
        return dishes.empty();
    }
};



class Waiter
{
private:
    std::random_device rd;

    std::default_random_engine dre;

    //Диапазон блюд
    const int uidDishMin = 0;
    const int uidDishMax = static_cast<int>(Dish::MAX) - 1;

    //Что закажет клиент
    std::uniform_int_distribution<int> uidDish;

    //Диапазон времени оформления заказа
    const int uidOrderMin;
    const int uidOrderMax;

    //Время оформления заказа
    std::uniform_int_distribution<int> uidOrder;
public:
    Waiter(const int in_uidOrderMin, const int in_uidOrderMax) :
            uidOrderMin{ in_uidOrderMin }, uidOrderMax{ in_uidOrderMax }, dre{ rd() },
            uidDish{ std::uniform_int_distribution<>(uidDishMin, uidDishMax) },
            uidOrder{ std::uniform_int_distribution<>(uidOrderMin, uidOrderMax) } {}

    //Официант оформляет заказ
    std::chrono::seconds orderMaking()
    {
        return static_cast<std::chrono::seconds>(uidOrder(dre));
    }

    //Официант определяет блюдо
    Dish dishDifining()
    {
        //Случайное блюдо
        return static_cast<Dish>(uidDish(dre));
    }
};



class Kitchen
{
private:
    std::random_device rd;

    std::default_random_engine dre;

    //Диапазон времени приготовления блюда
    const int uidMin;
    const int uidMax;

    //Время приготовления блюда
    std::uniform_int_distribution<int> uid;
public:
    Kitchen(const int in_uidMin, const int in_uidMax) :
            uidMin{ in_uidMin }, uidMax{ in_uidMax }, dre{ rd() },
            uid{std::uniform_int_distribution<>(uidMin, uidMax)} {}

    //Кухня готовит блюдо
    std::chrono::seconds dishPrepares()
    {
        return static_cast<std::chrono::seconds>(uid(dre));
    }
};



class Courier
{
private:
    //Интервал времени, через который курьер забирает готовые блюда с кухни
    const std::chrono::seconds interval;
public:
    Courier(const std::chrono::seconds in_interval) : interval{in_interval} {}

    const std::chrono::seconds getInterval()
    {
        return interval;
    }
};