#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Скрипт для сбора корпуса документов по тематике Формулы-1, гоночных болидов и спортивных автомобилей
Источники: Wikipedia (несколько языков) + Fandom Wiki (F1 Wiki)
"""

import os
import json
import time
import requests
from collections import deque

# ============= КОНФИГУРАЦИЯ =============

SAVE_DIR = "./f1_racing_corpus_data"

MAX_PAGES = 100_000
BATCH_SIZE = 15_000

API_DELAY = 0.15
PAGE_DELAY = 0.08

MIN_TEXT_CHARS = 300

PAGES_LIST_PATH = os.path.join(SAVE_DIR, "pages_list.json")

# ============= ИСТОЧНИКИ ДАННЫХ =============

SOURCES = [
    # Источник 1: Wikipedia (русский)
    {
        "source": "wikipedia_ru",
        "api_url": "https://ru.wikipedia.org/w/api.php",
        "lang": "ru",
        "start_categories": [
            "Категория:Формула-1",
            "Категория:Команды Формулы-1",
            "Категория:Пилоты Формулы-1",
            "Категория:Гоночные автомобили",
            "Категория:Суперкары",
            "Категория:Спортивные автомобили",
            "Категория:Автоспорт",
            "Категория:Гран-при Формулы-1",
        ],
    },
    # Источник 2: Wikipedia (английский)
    {
        "source": "wikipedia_en",
        "api_url": "https://en.wikipedia.org/w/api.php",
        "lang": "en",
        "start_categories": [
            "Category:Formula One",
            "Category:Formula One cars",
            "Category:Formula One teams",
            "Category:Formula One World Drivers' Champions",
            "Category:Formula One Grands Prix",
            "Category:Sports car racing",
            "Category:Sports cars",
            "Category:Racing cars",
            "Category:Supercars",
            "Category:Motorsport",
        ],
    },
    # Источник 3: Wikipedia (немецкий)
    {
        "source": "wikipedia_de",
        "api_url": "https://de.wikipedia.org/w/api.php",
        "lang": "de",
        "start_categories": [
            "Kategorie:Formel 1",
            "Kategorie:Formel-1-Rennwagen",
            "Kategorie:Formel-1-Team",
            "Kategorie:Sportwagen",
            "Kategorie:Supersportwagen",
        ],
    },
    # Источник 4: Wikipedia (испанский)
    {
        "source": "wikipedia_es",
        "api_url": "https://es.wikipedia.org/w/api.php",
        "lang": "es",
        "start_categories": [
            "Categoría:Fórmula 1",
            "Categoría:Equipos de Fórmula 1",
            "Categoría:Pilotos de Fórmula 1",
            "Categoría:Automóviles deportivos",
        ],
    },
    # Источник 5: Fandom Wiki (F1 Wiki) - ДРУГОЙ СПОСОБ СКАЧИВАНИЯ
    {
        "source": "f1_fandom",
        "api_url": "https://f1.fandom.com/api.php",
        "lang": "en",
        "start_categories": [
            "Category:Formula One",
            "Category:Cars",
            "Category:Teams",
            "Category:Drivers",
            "Category:Circuits",
            "Category:Engines",
            "Category:Seasons",
            "Category:Races",
        ],
    },
]

# ============= HTTP СЕССИЯ =============

session = requests.Session()
session.headers.update({
    "User-Agent": "F1RacingCorpus/2.0 (Educational research project; contact: student@university.edu)"
})

# ============= ФУНКЦИИ ДЛЯ РАБОТЫ С API =============

def fetch_category_members(api_url, cat, cmcontinue=None):
    """
    Получает список страниц и подкатегорий из категории
    """
    params = {
        "action": "query",
        "list": "categorymembers",
        "cmtitle": cat,
        "cmtype": "page|subcat",
        "cmlimit": "max",
        "format": "json",
    }
    if cmcontinue:
        params["cmcontinue"] = cmcontinue
    
    try:
        r = session.get(api_url, params=params, timeout=30)
        r.raise_for_status()
        data = r.json()
        members = data.get("query", {}).get("categorymembers", [])
        next_token = data.get("continue", {}).get("cmcontinue")
        return members, next_token
    except Exception as e:
        print(f"[!] Ошибка при запросе категории {cat}: {e}")
        return [], None

def get_plain_text(api_url, pageid):
    """
    Получает текстовое содержимое страницы по её ID
    """
    params = {
        "action": "query",
        "pageids": pageid,
        "prop": "extracts",
        "explaintext": True,
        "format": "json",
    }
    
    try:
        r = session.get(api_url, params=params, timeout=40)
        r.raise_for_status()
        pages = r.json().get("query", {}).get("pages", {})
        p = pages.get(str(pageid), {})
        return p.get("title", ""), p.get("extract", "")
    except Exception as e:
        print(f"[!] Ошибка при загрузке текста страницы {pageid}: {e}")
        return "", ""

def get_fandom_text(api_url, pageid):
    """
    АЛЬТЕРНАТИВНЫЙ СПОСОБ: Получает текст из Fandom Wiki через parse API
    Отличается от обычной Wikipedia тем, что использует action=parse
    """
    # Сначала получаем название страницы
    params_info = {
        "action": "query",
        "pageids": pageid,
        "format": "json",
    }
    
    try:
        r = session.get(api_url, params=params_info, timeout=40)
        r.raise_for_status()
        pages = r.json().get("query", {}).get("pages", {})
        page_data = pages.get(str(pageid), {})
        title = page_data.get("title", "")
        
        if not title:
            return "", ""
        
        # Теперь парсим содержимое через parse API (специфично для Fandom)
        params_parse = {
            "action": "parse",
            "pageid": pageid,
            "prop": "wikitext",
            "format": "json",
        }
        
        r2 = session.get(api_url, params=params_parse, timeout=40)
        r2.raise_for_status()
        parse_data = r2.json().get("parse", {})
        wikitext = parse_data.get("wikitext", {}).get("*", "")
        
        # Простая очистка wikitext от разметки
        text = clean_wikitext(wikitext)
        
        return title, text
    
    except Exception as e:
        # Fallback на стандартный метод
        return get_plain_text(api_url, pageid)

def clean_wikitext(wikitext):
    """
    Очищает wikitext от разметки (упрощенная версия)
    """
    import re
    
    text = wikitext
    # Удаляем шаблоны
    text = re.sub(r'\{\{[^}]*\}\}', '', text)
    # Удаляем ссылки, оставляя текст
    text = re.sub(r'\[\[(?:[^|\]]*\|)?([^\]]+)\]\]', r'\1', text)
    # Удаляем внешние ссылки
    text = re.sub(r'\[http[^\]]*\]', '', text)
    # Удаляем разметку заголовков
    text = re.sub(r'==+\s*([^=]+)\s*==+', r'\n\1\n', text)
    # Удаляем жирный и курсив
    text = re.sub(r"'{2,}", '', text)
    # Удаляем HTML комментарии
    text = re.sub(r'<!--.*?-->', '', text, flags=re.DOTALL)
    # Удаляем ссылки на файлы
    text = re.sub(r'\[\[File:.*?\]\]', '', text, flags=re.IGNORECASE)
    
    return text.strip()

# ============= СБОР СПИСКА СТРАНИЦ =============

def collect_pages_multi_source():
    """
    Собирает список страниц из всех источников методом обхода в ширину (BFS)
    """
    pages = {}
    visited = set()
    q = deque()
    
    print("="*60)
    print("НАЧАЛО СБОРА СПИСКА СТРАНИЦ")
    print("="*60)
    
    # Добавляем начальные категории из всех источников
    for s in SOURCES:
        for c in s["start_categories"]:
            q.append((s["source"], s["api_url"], s["lang"], c))
    
    print(f"Начальных категорий в очереди: {len(q)}")
    print(f"Целевое количество страниц: {MAX_PAGES:,}\n")
    
    while q and len(pages) < MAX_PAGES:
        source, api_url, lang, cat = q.popleft()
        key = (source, cat)
        
        if key in visited:
            continue
        
        visited.add(key)
        
        # Выводим прогресс
        if len(visited) % 10 == 0:
            print(f"[{len(pages):6,}/{MAX_PAGES:,}] Обработано категорий: {len(visited):4,} | "
                  f"Источник: {source:15s} | Категория: {cat[:40]}")
        
        cmc = None
        while True:
            try:
                members, cmc = fetch_category_members(api_url, cat, cmc)
            except Exception:
                break
            
            for m in members:
                ns = m.get("ns")
                title = m.get("title", "")
                pageid = m.get("pageid")
                
                # Namespace 14 = категория
                if ns == 14 and title:
                    q.append((source, api_url, lang, title))
                
                # Namespace 0 = обычная статья
                elif ns == 0 and pageid is not None:
                    doc_key = (source, int(pageid))
                    if doc_key not in pages:
                        pages[doc_key] = {
                            "pageid": int(pageid),
                            "title": title,
                            "lang": lang,
                            "source": source,
                            "api_url": api_url
                        }
                        
                        if len(pages) >= MAX_PAGES:
                            break
            
            if len(pages) >= MAX_PAGES:
                break
            
            if not cmc:
                break
            
            time.sleep(API_DELAY)
    
    print("\n" + "="*60)
    print(f"✓ СБОР ЗАВЕРШЕН")
    print(f"  Найдено страниц: {len(pages):,}")
    print(f"  Обработано категорий: {len(visited):,}")
    print("="*60 + "\n")
    
    return list(pages.values())

# ============= СОХРАНЕНИЕ/ЗАГРУЗКА СПИСКА =============

def save_pages_list(pages_list):
    """Сохраняет список страниц в JSON файл"""
    with open(PAGES_LIST_PATH, "w", encoding="utf-8") as f:
        json.dump(pages_list, f, ensure_ascii=False, indent=2)
    print(f"✓ Список страниц сохранен: {PAGES_LIST_PATH}")

def load_pages_list():
    """Загружает список страниц из JSON файла"""
    with open(PAGES_LIST_PATH, "r", encoding="utf-8") as f:
        return json.load(f)

# ============= СКАЧИВАНИЕ ТЕКСТОВ =============

def download_texts(pages_list):
    """
    Загружает текстовое содержимое страниц и сохраняет в батчи
    """
    os.makedirs(SAVE_DIR, exist_ok=True)
    
    # Определяем существующие батчи
    existing_batches = [
        name for name in os.listdir(SAVE_DIR)
        if name.startswith("batch_") and name.endswith(".jsonl")
    ]
    
    if existing_batches:
        max_batch = max(int(name[6:10]) for name in existing_batches)
        batch_id = max_batch + 1
    else:
        batch_id = 1
    
    # Подсчитываем уже обработанные документы
    processed = 0
    for name in existing_batches:
        path = os.path.join(SAVE_DIR, name)
        try:
            with open(path, "r", encoding="utf-8") as f:
                for _ in f:
                    processed += 1
        except Exception:
            pass
    
    print("="*60)
    print("НАЧАЛО ЗАГРУЗКИ ТЕКСТОВ")
    print("="*60)
    print(f"Уже обработано: {processed:,} документов")
    print(f"Осталось обработать: {len(pages_list) - processed:,}")
    print(f"Текущий батч: {batch_id:04d}\n")
    
    current_batch_path = os.path.join(SAVE_DIR, f"batch_{batch_id:04d}.jsonl")
    f = open(current_batch_path, "a", encoding="utf-8")
    counter_in_batch = processed % BATCH_SIZE
    
    total_pages = len(pages_list)
    saved_count = 0
    skipped_count = 0
    
    for idx, page in enumerate(pages_list):
        if idx < processed:
            continue
        
        pageid = page["pageid"]
        source = page.get("source", "wikipedia_ru")
        lang = page.get("lang", "ru")
        api_url = page.get("api_url")
        
        if not api_url:
            continue
        
        try:
            # РАЗНЫЕ СПОСОБЫ ЗАГРУЗКИ для разных источников
            if source == "f1_fandom":
                # Альтернативный способ для Fandom Wiki
                title, text = get_fandom_text(api_url, pageid)
            else:
                # Стандартный способ для Wikipedia
                title, text = get_plain_text(api_url, pageid)
        
        except Exception as e:
            print(f"[!] Ошибка загрузки {pageid}: {e}")
            skipped_count += 1
            continue
        
        # Фильтруем слишком короткие тексты
        if not text or len(text) < MIN_TEXT_CHARS:
            skipped_count += 1
            continue
        
        # Формируем документ
        doc = {
            "pageid": pageid,
            "title": title,
            "lang": lang,
            "source": source,
            "text": text,
            "text_length": len(text)
        }
        
        # Записываем в файл
        f.write(json.dumps(doc, ensure_ascii=False) + "\n")
        f.flush()
        
        counter_in_batch += 1
        saved_count += 1
        
        # Создаем новый батч при необходимости
        if counter_in_batch >= BATCH_SIZE:
            f.close()
            print(f"\n✓ Батч {batch_id:04d} завершен ({BATCH_SIZE:,} документов)")
            batch_id += 1
            current_batch_path = os.path.join(SAVE_DIR, f"batch_{batch_id:04d}.jsonl")
            f = open(current_batch_path, "a", encoding="utf-8")
            counter_in_batch = 0
        
        # Выводим прогресс каждые 100 документов
        if (idx + 1) % 100 == 0:
            progress = (idx + 1) / total_pages * 100
            print(f"[{idx+1:6,}/{total_pages:,}] {progress:5.1f}% | "
                  f"Сохранено: {saved_count:6,} | Пропущено: {skipped_count:5,} | "
                  f"Источник: {source:15s}")
        
        time.sleep(PAGE_DELAY)
    
    f.close()
    
    print("\n" + "="*60)
    print("✓ ЗАГРУЗКА ЗАВЕРШЕНА")
    print(f"  Всего сохранено: {saved_count:,} документов")
    print(f"  Пропущено: {skipped_count:,}")
    print(f"  Итоговых батчей: {batch_id}")
    print("="*60 + "\n")

# ============= ГЛАВНАЯ ФУНКЦИЯ =============

def main():
    """
    Основная функция запуска
    """
    os.makedirs(SAVE_DIR, exist_ok=True)
    
    print("\n" + "="*60)
    print(" "*10 + "F1 RACING CORPUS COLLECTOR")
    print("="*60)
    print(f"Целевой объем: {MAX_PAGES:,} документов")
    print(f"Размер батча: {BATCH_SIZE:,} документов")
    print(f"Минимальная длина текста: {MIN_TEXT_CHARS} символов")
    print(f"Директория сохранения: {SAVE_DIR}")
    print(f"Количество источников: {len(SOURCES)}")
    print("="*60 + "\n")
    
    # Проверяем, нужно ли собирать список страниц
    need_collect = True
    if os.path.exists(PAGES_LIST_PATH):
        try:
            tmp = load_pages_list()
            if isinstance(tmp, list) and len(tmp) > 0:
                print(f"✓ Найден существующий список страниц ({len(tmp):,} страниц)")
                need_collect = False
        except Exception:
            print("[!] Ошибка чтения списка страниц, будет создан новый")
            need_collect = True
    
    # Собираем список страниц если нужно
    if need_collect:
        print("→ Начинается сбор списка страниц...\n")
        pages_list = collect_pages_multi_source()
        save_pages_list(pages_list)
    else:
        print("→ Используется существующий список страниц\n")
    
    # Загружаем список
    pages_list = load_pages_list()
    
    if len(pages_list) == 0:
        print("[!] ОШИБКА: Список страниц пуст!")
        return
    
    print(f"→ Загружен список из {len(pages_list):,} страниц\n")
    
    # Начинаем загрузку текстов
    download_texts(pages_list)
    
    print("\n" + "="*60)
    print(" "*15 + "🏁 ГОТОВО!")
    print("="*60)
    print(f"Результаты сохранены в: {SAVE_DIR}")
    print("="*60 + "\n")

# ============= ТОЧКА ВХОДА =============

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n[!] Прервано пользователем. Прогресс сохранен.")
    except Exception as e:
        print(f"\n\n[!] КРИТИЧЕСКАЯ ОШИБКА: {e}")
        import traceback
        traceback.print_exc()
