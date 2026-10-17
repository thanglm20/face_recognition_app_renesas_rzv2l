#pragma once
#ifndef FACE_GALLERY_H
#define FACE_GALLERY_H

#include <cmath>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../app_config.h"

/* Cosine similarity gallery of enrolled face embeddings (thread safe). */
class FaceGallery
{
public:
    struct Match
    {
        std::string name;
        float similarity;
        bool known;
    };

    static FaceGallery& instance()
    {
        static FaceGallery g;
        return g;
    }

    static void normalize(std::vector<float>& v)
    {
        double s = 0;
        for (float x : v) s += (double)x * x;
        s = std::sqrt(s);
        if (s < 1e-12) return;
        for (float& x : v) x = (float)(x / s);
    }

    /* Both vectors must be L2-normalized, so the dot product is the cosine. */
    static float cosine(const std::vector<float>& a, const std::vector<float>& b)
    {
        if (a.size() != b.size() || a.empty()) return -1.0f;
        float d = 0;
        for (size_t i = 0; i < a.size(); i++) d += a[i] * b[i];
        return d;
    }

    Match identify(const std::vector<float>& emb)
    {
        std::lock_guard<std::mutex> lock(mtx);
        Match best{"Unknown", -1.0f, false};
        std::string best_name;
        for (const auto& e : entries)
        {
            float s = cosine(emb, e.emb);
            if (s > best.similarity)
            {
                best.similarity = s;
                best_name = e.name;
            }
        }
        best.known = (best.similarity >= threshold);
        if (best.known) best.name = best_name;
        return best;
    }

    bool enroll(const std::string& name, const std::vector<float>& emb)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (name.empty() || emb.empty()) return false;
        entries.push_back({name, emb});
        if (save_locked()) return true;
        entries.pop_back();
        return false;
    }

    bool remove(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mtx);
        const std::vector<Entry> previous = entries;
        bool removed = false;
        for (size_t i = entries.size(); i > 0; i--)
        {
            if (entries[i - 1].name == name)
            {
                entries.erase(entries.begin() + (i - 1));
                removed = true;
            }
        }
        if (!removed) return false;
        if (save_locked()) return true;
        entries = previous;
        return false;
    }

    bool clear()
    {
        std::lock_guard<std::mutex> lock(mtx);
        const std::vector<Entry> previous = entries;
        entries.clear();
        if (save_locked())
        {
            pending_name.clear();
            return true;
        }
        entries = previous;
        return false;
    }

    void request_enroll(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mtx);
        pending_name = name;
    }

    /* Returns true and the name if an enrollment is waiting. */
    bool take_pending(std::string& name)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (pending_name.empty()) return false;
        name = pending_name;
        pending_name.clear();
        return true;
    }

    void set_threshold(float t) { std::lock_guard<std::mutex> lock(mtx); threshold = t; }

    void load(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(mtx);
        file_path = path;
        entries.clear();
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line))
        {
            std::istringstream ss(line);
            Entry e;
            if (!(ss >> e.name)) continue;
            float x;
            while (ss >> x) e.emb.push_back(x);
            if (!e.emb.empty()) entries.push_back(e);
        }
    }

private:
    struct Entry
    {
        std::string name;
        std::vector<float> emb;
    };

    bool save_locked()
    {
        if (file_path.empty()) return false;
        const std::string temp_path = file_path + ".tmp";
        std::ofstream f(temp_path, std::ios::trunc);
        if (!f) return false;
        f.precision(9);
        for (const auto& e : entries)
        {
            f << e.name;
            for (float x : e.emb) f << ' ' << x;
            f << '\n';
        }
        f.close();
        if (!f || std::rename(temp_path.c_str(), file_path.c_str()) != 0)
        {
            std::remove(temp_path.c_str());
            return false;
        }
        return true;
    }

    std::mutex mtx;
    std::vector<Entry> entries;
    std::string pending_name;
    std::string file_path = "face_gallery.txt";
    float threshold = app_config::FACE_MATCH_THRESHOLD;
};

#endif
