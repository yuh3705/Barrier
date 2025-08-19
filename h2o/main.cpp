#include <SFML/Graphics.hpp>
#include <bits/stdc++.h>
using namespace std;

const double PI = 2 * asin(1.0);
const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;
const double BARRIER_X = 300.0;
const double FINISH_X  = 1000.0;

// đèn báo để giới hạn 2H và 1O vào barrier
static counting_semaphore<2> semH(2);
static counting_semaphore<1> semO(1);

// 1 nguyên tố
struct Element {
    char type;
    double x, y;
    double radius;
    double speed;
    bool flag;        // cờ hiệu đã ghép nhóm hay chưa
    bool finished;    // cờ hiệu đã về đích hay chưa

    Element(char t, double yy, double spd): type(t), x(0.0), y(yy), speed(spd), flag(false), finished(false)
    {
        radius = (t == 'H' ? 15.0 : 25.0);
    }

    // tâm hình tròn
    sf::Vector2f center() const {
        return { static_cast<float>(x + radius), static_cast<float>(y + radius) };
    }
};

// 1 phân tử H2O được ghép
struct Link_Group {
    Element* o;
    Element* h1;
    Element* h2;
};

// Danh sách các phần tử chưa về đích
static vector<Element*> Elements;
static mutex Elements_mutex;

// Danh sách các phần tử đang đợi ở barrier
static vector<Element*> pendingElements;
static mutex pending_mutex;

// Danh sách các nhóm đã ghép
static vector<Link_Group> Link;
static mutex Link_mutex;

// Barrier kích cỡ 3 khi đủ 3 luồng dến thì chạy
static barrier sync_barrier(3, [](){
    lock_guard<mutex> lock(pending_mutex);

    if (pendingElements.size() >= 3) {
        Element* o_ptr = nullptr;
        Element* h_ptr1 = nullptr;
        Element* h_ptr2 = nullptr;

        // Tìm 1 O đầu tiên
        for (Element* a : pendingElements) {
            if (a->type == 'O' && o_ptr == nullptr) {
                o_ptr = a;
            }
        }
        int hcount = 0;
        // Tìm hai H đầu tiên
        for (Element* a : pendingElements) {
            if (a->type == 'H') {
                if (hcount == 0) {
                    h_ptr1 = a;
                    hcount++;
                } else if (hcount == 1) {
                    h_ptr2 = a;
                    hcount++;
                }
            }
            if (hcount == 2) break;
        }

        if (o_ptr && h_ptr1 && h_ptr2) {
            // Đánh dấu flag = true (đã bonded)
            o_ptr->flag  = true;
            h_ptr1->flag = true;
            h_ptr2->flag = true;

            // Tạo liên kết và push vào vector Link
            {
                lock_guard<mutex> lock2(Link_mutex);
                Link.push_back({ o_ptr, h_ptr1, h_ptr2 });
            }

            // Xóa 3 Element ấy khỏi pendingElements
            pendingElements.erase(
                remove(pendingElements.begin(), pendingElements.end(), o_ptr),
                pendingElements.end()
            );
            pendingElements.erase(
                remove(pendingElements.begin(), pendingElements.end(), h_ptr1),
                pendingElements.end()
            );
            pendingElements.erase(
                remove(pendingElements.begin(), pendingElements.end(), h_ptr2),
                pendingElements.end()
            );
        }
    }
});

// Sinh ra các H và O
double pickRandomY(char type) {
    static thread_local mt19937 rng(random_device{}());
    uniform_real_distribution<double> distH(50.0, 200.0);
    uniform_real_distribution<double> distO(300.0, 500.0);

    double yy;
    bool ok = false;
    while (!ok) {
        yy = (type == 'H' ? distH(rng) : distO(rng));
        ok = true;

        // Kiểm tra với các Element đang có để tránh lặp
        lock_guard<mutex> lock(Elements_mutex);
        for (auto* a : Elements) {
            double dy = abs(a->y - yy);
            double sumR = a->radius + (type == 'H' ? 15.0 : 25.0);
            if (dy < sumR) {
                ok = false;
                break;
            }
        }
    }
    return yy;
}

// Vẽ mũi tên liên kết 2H và 1O
void drawArrow(sf::RenderWindow& window, sf::Vector2f A, sf::Vector2f B) {

    // thân mũi tên
    sf::VertexArray line(sf::PrimitiveType::Lines, 2);
    line[0].position = A;
    line[0].color    = sf::Color::White;
    line[1].position = B;
    line[1].color    = sf::Color::White;
    window.draw(line);

    //Tính vector đơn vị từ A→B (dir) và chiều dài (len)
    sf::Vector2f dir = B - A;
    double len = hypot(dir.x, dir.y);
    if (len == 0) return;
    dir /= static_cast<float>(len);

    // mũi tên
    double arrowLength = 15.0;
    double angleOffset = 20.0 * PI/180.0;

    // vẽ mũi tên B->A
    {
        double thetaB = atan2(dir.y, dir.x);
        double thetaB1 = thetaB + PI - angleOffset;
        double thetaB2 = thetaB + PI + angleOffset;

        sf::Vector2f pB1 = {
            static_cast<float>(B.x + arrowLength * cos(thetaB1)),
            static_cast<float>(B.y + arrowLength * sin(thetaB1))
        };
        sf::Vector2f pB2 = {
            static_cast<float>(B.x + arrowLength * cos(thetaB2)),
            static_cast<float>(B.y + arrowLength * sin(thetaB2))
        };

        sf::VertexArray headB1(sf::PrimitiveType::Lines, 2);
        headB1[0].position = B; headB1[0].color = sf::Color::White;
        headB1[1].position = pB1; headB1[1].color = sf::Color::White;
        window.draw(headB1);

        sf::VertexArray headB2(sf::PrimitiveType::Lines, 2);
        headB2[0].position = B; headB2[0].color = sf::Color::White;
        headB2[1].position = pB2; headB2[1].color = sf::Color::White;
        window.draw(headB2);
    }

    // vẽ mũi tên A->B
    {
        double thetaA = atan2(dir.y, dir.x);
        double thetaA1 = thetaA + angleOffset;
        double thetaA2 = thetaA - angleOffset;

        sf::Vector2f pA1 = {
            static_cast<float>(A.x + arrowLength * cos(thetaA1)),
            static_cast<float>(A.y + arrowLength * sin(thetaA1))
        };
        sf::Vector2f pA2 = {
            static_cast<float>(A.x + arrowLength * cos(thetaA2)),
            static_cast<float>(A.y + arrowLength * sin(thetaA2))
        };

        sf::VertexArray headA1(sf::PrimitiveType::Lines, 2);
        headA1[0].position = A; headA1[0].color = sf::Color::White;
        headA1[1].position = pA1; headA1[1].color = sf::Color::White;
        window.draw(headA1);

        sf::VertexArray headA2(sf::PrimitiveType::Lines, 2);
        headA2[0].position = A; headA2[0].color = sf::Color::White;
        headA2[1].position = pA2; headA2[1].color = sf::Color::White;
        window.draw(headA2);
    }
}

void hydrogen_thread() {
    // sinh ngẫu nhiên tốc độ và vị trí
    static thread_local mt19937 rng(random_device{}());
    uniform_real_distribution<double> speedDist(2.2, 3.5);
    double spd = speedDist(rng);
    double yy  = pickRandomY('H');

    // sinh 1 phần tử mới, add vào Elements
    Element* a = new Element('H', yy, spd);
    {
        lock_guard<mutex> lock(Elements_mutex);
        Elements.push_back(a);
    }

    // Di chuyển đến barrier
    while (true) {
        {
            lock_guard<mutex> lock(Elements_mutex);
            if (a->x + 2*a->radius >= BARRIER_X) break;
            a->x += a->speed;
        }
        this_thread::sleep_for(chrono::milliseconds(16));
    }

    // đến barrier, giảm giá trị của đèn báo
    semH.acquire();

    // add vào pendingElements
    {
        lock_guard<mutex> lock(pending_mutex);
        pendingElements.push_back(a);
    }
    sync_barrier.arrive_and_wait();

    // đã đủ, giải phóng đèn báo
    semH.release();
    a -> speed *= 5;    // tăng tốc cho nhanh

    // di chuyển tiếp
    while (true) {
        {
            lock_guard<mutex> lock(Elements_mutex);
            if (a->x >= FINISH_X) break;
            a->x += a->speed;
        }
        this_thread::sleep_for(chrono::milliseconds(16));
    }

    // đánh dấu đã về đích
    a->finished = true;

    // Xóa Link_Group nào có chứa a
    {
        lock_guard<mutex> lock(Link_mutex);
        Link.erase(
            remove_if(Link.begin(), Link.end(),
                      [&](const Link_Group& bg) {
                          return (bg.o == a) || (bg.h1 == a) || (bg.h2 == a);
                      }),
            Link.end()
        );
    }

    // xóa khỏi Elements
    {
        lock_guard<mutex> lock(Elements_mutex);
        auto it = find(Elements.begin(), Elements.end(), a);
        if (it != Elements.end()) Elements.erase(it);
    }
    delete a;
}

void oxygen_thread() {
    // sinh ngẫu nhiên tốc độ và vị trí
    static thread_local mt19937 rng(random_device{}());
    uniform_real_distribution<double> speedDist(2.2, 3.5);
    double spd = speedDist(rng);
    double yy  = pickRandomY('O');

    // sinh 1 phần tử mới, add vào Elements
    Element* a = new Element('O', yy, spd);
    {
        lock_guard<mutex> lock(Elements_mutex);
        Elements.push_back(a);
    }

    // Di chuyển đến barrier
    while (true) {
        {
            lock_guard<mutex> lock(Elements_mutex);
            if (a->x + 2*a->radius >= BARRIER_X) break;
            a->x += a->speed;
        }
        this_thread::sleep_for(chrono::milliseconds(16));
    }

    // đến barrier, giảm giá trị của đèn báo
    semO.acquire();

    // add vào pendingElements
    {
        lock_guard<mutex> lock(pending_mutex);
        pendingElements.push_back(a);
    }
    sync_barrier.arrive_and_wait();

    // đã đủ, giải phóng đèn báo
    semO.release();
    a->speed *= 5;  // tăng tốc

    // Di chuyển tiếp
    while (true) {
        {
            lock_guard<mutex> lock(Elements_mutex);
            if (a->x - a->radius >= FINISH_X) break;
            a->x += a->speed;
        }
        this_thread::sleep_for(chrono::milliseconds(16));
    }

    // Đánh dấu đã về đích
    a->finished = true;

    // Xóa Link_Group nào có chứa a
    {
        lock_guard<mutex> lock(Link_mutex);
        Link.erase(
            remove_if(Link.begin(), Link.end(),
                      [&](const Link_Group& bg) {
                          return (bg.o == a) || (bg.h1 == a) || (bg.h2 == a);
                      }),
            Link.end()
        );
    }

    // xóa khỏi Elements
    {
        lock_guard<mutex> lock(Elements_mutex);
        auto it = find(Elements.begin(), Elements.end(), a);
        if (it != Elements.end()) Elements.erase(it);
    }
    delete a;
}

// Liên tục sinh ra H và O mới
void spawn_Elements() {
    static thread_local mt19937 rng(random_device{}());
    uniform_int_distribution<int> dist(0, 2);

    while (true) {
        int x = dist(rng);
        if (x < 2) {
            thread(hydrogen_thread).detach();
        } else {
            thread(oxygen_thread).detach();
        }
        this_thread::sleep_for(chrono::milliseconds(300));
    }
}

int main() {
    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Demo H2O");
    window.setFramerateLimit(60);

    // Start thread để liên tục sinh Element
    thread(spawn_Elements).detach();

    while (window.isOpen()) {
         while (const optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        window.clear(sf::Color::Black);

        // Vẽ vạch barrier
        sf::RectangleShape barrierLine;
        barrierLine.setSize({4.f, static_cast<float>(WINDOW_HEIGHT)});
        barrierLine.setPosition({static_cast<float>(BARRIER_X), 0.f});
        barrierLine.setFillColor(sf::Color(150, 150, 150));
        window.draw(barrierLine);

        // Vẽ tất cả Elements
        {
            lock_guard<mutex> lock(Elements_mutex);
            for (auto* a : Elements) {
                sf::CircleShape circle(static_cast<float>(a->radius));

                // nếu chưa đến barrier thì H màu cyan, O màu đỏ
                if (!a->flag) {
                    if (a->type == 'H')
                        circle.setFillColor(sf::Color::Cyan);
                    else
                        circle.setFillColor(sf::Color::Red);
                }

                // Qua barrier thì chuyển thành xanh thể hiện đã liên kết
                else {
                    circle.setFillColor(sf::Color::Green);
                }
                circle.setPosition({static_cast<float>(a->x), static_cast<float>(a->y)});
                window.draw(circle);
            }
        }

        // Vẽ các mũi tên liên kết
        {
            lock_guard<mutex> lock(Link_mutex);
            for (auto& bg : Link) {
                // đi hết đường rồi thì xóa mũi tên
                if (bg.h1->finished || bg.h2->finished || bg.o->finished)
                    continue;

                // Toạ độ tâm của O, H1, H2
                sf::Vector2f centerO  = bg.o->center();
                sf::Vector2f centerH1 = bg.h1->center();
                sf::Vector2f centerH2 = bg.h2->center();

                // vẽ mũi tên
                drawArrow(window, centerO, centerH1);
                drawArrow(window, centerO, centerH2);
            }
        }

        window.display();
    }

    return 0;
}
