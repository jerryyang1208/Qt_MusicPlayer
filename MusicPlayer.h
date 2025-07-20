#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QWidget>
#include <QStandardItemModel>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSlider>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MusicPlayer;
}
QT_END_NAMESPACE

class MusicPlayer : public QWidget
{
    Q_OBJECT

public:
    MusicPlayer(QWidget *parent = nullptr);
    ~MusicPlayer();

private slots:
    void on_openDirBtn_clicked();
    void on_musicListView_doubleClicked(const QModelIndex &index);
    void on_prevBtn_clicked();
    int getCurrentSongIndex() const;
    void on_nextBtn_clicked();
    void on_volumeBtn_clicked();
    qreal linearToLogVolume(int linearVolume);
    void on_volumeBar_sliderMoved(int value);
    void on_playBtn_clicked();
    void updatePlayButtonIcon(QMediaPlayer::PlaybackState state);
    void updateDurationLab(qint64 duration); // 更新歌曲总时长显示
    void updatePlayDurLab(qint64 position);  // 更新当前播放位置时长显示
    void on_playSlider_sliderMoved(int position); // 拖动进度条时改变播放位置

private:
    Ui::MusicPlayer *ui;
    QStandardItemModel* m_listModel;
    QMediaPlayer* m_mediaPlayer{};
    QAudioOutput* m_audioOutput{};
    QSlider* volumeSlider;
    QWidget* volumeWidget; // 用于容纳音量滑块的浮动窗口
    int currentSongIndex;  // 当前播放歌曲的索引
    QTimer* m_timer;       // 用于定时更新进度条
};
#endif // MUSICPLAYER_H
