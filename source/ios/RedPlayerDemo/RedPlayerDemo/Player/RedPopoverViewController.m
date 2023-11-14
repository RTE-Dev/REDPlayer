//
//  RedPopoverViewController.m
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/4.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

#import "RedPopoverViewController.h"
@import Masonry;
@interface RedPopoverCell: UITableViewCell
@end

@implementation RedPopoverCell
- (void)setSelected:(BOOL)selected animated:(BOOL)animated {
    [super setSelected:selected animated:animated];
    if (selected) {
        self.textLabel.textColor = UIColor.redColor;
    } else {
        self.textLabel.textColor = UIColor.labelColor;
    }
}

- (void)setHighlighted:(BOOL)highlighted animated:(BOOL)animated {
    [super setHighlighted:highlighted animated:animated];
    if (highlighted) {
        self.textLabel.textColor = UIColor.redColor;
    } else {
        self.textLabel.textColor = UIColor.labelColor;
    }
}
- (void)layoutSubviews {
    [super layoutSubviews];
    
    if (self.imageView.image) {
        // Set imageView width
        CGRect imageViewFrame = self.imageView.frame;
        imageViewFrame.size.width = 50; // Change this to your desired width
        self.imageView.frame = imageViewFrame;
        // Adjust textLabel position
        CGRect textLabelFrame = self.textLabel.frame;
        textLabelFrame.origin.x = CGRectGetMaxX(imageViewFrame) + 10; // 10 is the spacing between imageView and textLabel
        self.textLabel.frame = textLabelFrame;
    }
}
@end

@interface RedPopoverViewController () <UITableViewDelegate, UITableViewDataSource>

@property (nonatomic, strong) NSArray<NSString *> *options;
@property (nonatomic, strong) NSArray<UIImage *> *icons;
@property (nonatomic, copy) PopoverOptionBlock completionHandler;
@property (nonatomic, strong) UITableView *tableView;
@property (nonatomic, assign) NSInteger selectedIndex;

@end

@implementation RedPopoverViewController

- (instancetype)initWithOptions:(NSArray<NSString *> *)options
                          icons:(nullable NSArray<UIImage *> *)icons
                  selectedIndex:(NSInteger)selectedIndex
              completionHandler:(PopoverOptionBlock)completionHandler {
    self = [super init];
    if (self) {
        _options = options;
        _icons = icons;
        _selectedIndex = selectedIndex;
        _completionHandler = completionHandler;
    }
    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    CGSize contentSize = CGSizeMake([self calculateMaxWidth], _options.count * [self cellHeight]);
    self.preferredContentSize = contentSize;
    self.tableView = [[UITableView alloc] initWithFrame:CGRectZero style:UITableViewStylePlain];
    self.tableView.showsVerticalScrollIndicator = NO;
    self.tableView.delegate = self;
    self.tableView.dataSource = self;
    self.tableView.backgroundColor = UIColor.clearColor;
    [self.view addSubview:self.tableView];
}

- (CGFloat)calculateMaxWidth {
    CGFloat maxWidth = 150.0;
    UIFont *font = [UIFont systemFontOfSize:17.0];
    for (NSString *option in _options) {
        CGSize size = [option sizeWithAttributes:@{NSFontAttributeName: font}];
        if (size.width > maxWidth) {
            maxWidth = size.width;
        }
    }
    if (_icons.count > 0) {
        maxWidth += 60;
    }
    CGFloat screenLimitWidth = [UIScreen mainScreen].bounds.size.width * 2.5 / 3.0;
    if (maxWidth > screenLimitWidth) {
        maxWidth = screenLimitWidth;
    }
    return maxWidth;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [self.tableView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.top.left.width.height.mas_equalTo(self.view);
    }];
    if (_selectedIndex < self.options.count) {
        [self.tableView reloadData];
        [self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:_selectedIndex inSection:0] atScrollPosition:UITableViewScrollPositionTop animated:NO];
    }
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return self.options.count;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    return [self cellHeight];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"optionCell"];
    if (!cell) {
        cell = [[RedPopoverCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"optionCell"];
    }
    cell.backgroundColor = UIColor.clearColor;
    cell.imageView.image = self.icons[indexPath.row];
    cell.imageView.layer.masksToBounds = YES;
    cell.imageView.contentMode = UIViewContentModeScaleAspectFill;
    cell.textLabel.text = self.options[indexPath.row];
    return cell;
}

- (void)tableView:(UITableView *)tableView willDisplayCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (_selectedIndex == indexPath.row) {
        cell.selected = YES;
    } else {
        cell.selected = NO;
    }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    if (self.completionHandler) {
        self.completionHandler(indexPath.row);
    }
    [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
}

- (CGFloat)cellHeight {
    if (self.icons.count > 0) {
        return 50;
    }
    return 40;
}
@end


