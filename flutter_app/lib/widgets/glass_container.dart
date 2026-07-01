import 'package:flutter/material.dart';

class GlassContainer extends StatelessWidget {
  final Widget child;
  final double? width, height;
  final EdgeInsetsGeometry? padding, margin;
  final BorderRadiusGeometry? borderRadius;
  final Color? borderColor;
  final Color? bgColor;
  final List<BoxShadow>? boxShadow;
  final Gradient? gradient;
  final VoidCallback? onTap;
  final Alignment? alignment;

  const GlassContainer({
    super.key,
    required this.child,
    this.width,
    this.height,
    this.padding,
    this.margin,
    this.borderRadius,
    this.borderColor,
    this.bgColor,
    this.boxShadow,
    this.gradient,
    this.onTap,
    this.alignment,
  });

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final color = borderColor ?? theme.colorScheme.primary.withAlpha(30);
    final bg = bgColor ?? theme.colorScheme.surface.withAlpha(180);
    final radius = borderRadius ?? BorderRadius.circular(16);
    return Container(
      width: width,
      height: height,
      margin: margin,
      alignment: alignment,
      decoration: BoxDecoration(
        gradient: gradient,
        color: gradient == null ? bg : null,
        borderRadius: radius,
        border: Border.all(color: color, width: 0.5),
        boxShadow: boxShadow ?? [
          BoxShadow(
            color: theme.colorScheme.primary.withAlpha(8),
            blurRadius: 20,
            spreadRadius: -2,
          ),
        ],
      ),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          onTap: onTap,
          borderRadius: borderRadius ?? BorderRadius.circular(16),
          splashColor: theme.colorScheme.primary.withAlpha(20),
          highlightColor: Colors.transparent,
          child: Padding(
            padding: padding ?? EdgeInsets.all(16),
            child: child,
          ),
        ),
      ),
    );
  }
}

class GlassButton extends StatelessWidget {
  final Widget child;
  final VoidCallback? onTap;
  final double? width, height;
  final Color? accentColor;

  const GlassButton({
    super.key,
    required this.child,
    this.onTap,
    this.width,
    this.height,
    this.accentColor,
  });

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final color = accentColor ?? theme.colorScheme.primary;
    return GlassContainer(
      width: width,
      height: height,
      padding: EdgeInsets.symmetric(horizontal: 20, vertical: 14),
      borderColor: color.withAlpha(50),
      bgColor: theme.colorScheme.surface.withAlpha(200),
      onTap: onTap,
      child: child,
    );
  }
}

class GlowingDot extends StatefulWidget {
  final Color color;
  final double size;
  final bool animate;

  const GlowingDot({
    super.key,
    this.color = Color(0xFF00FF88),
    this.size = 8,
    this.animate = true,
  });

  @override
  State<GlowingDot> createState() => _GlowingDotState();
}

class _GlowingDotState extends State<GlowingDot> with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _animation;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: Duration(seconds: 2),
    );
    _animation = Tween(begin: 0.3, end: 1.0).animate(
      CurvedAnimation(parent: _controller, curve: Curves.easeInOut),
    );
    if (widget.animate) {
      _controller.repeat(reverse: true);
    } else {
      _controller.value = 1.0;
    }
  }

  @override
  void didUpdateWidget(GlowingDot old) {
    super.didUpdateWidget(old);
    if (widget.animate && !_controller.isAnimating) {
      _controller.repeat(reverse: true);
    } else if (!widget.animate && _controller.isAnimating) {
      _controller.stop();
      _controller.value = 1.0;
    }
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _animation,
      builder: (_, __) => Container(
        width: widget.size,
        height: widget.size,
        decoration: BoxDecoration(
          shape: BoxShape.circle,
          color: widget.color,
          boxShadow: [
            BoxShadow(
              color: widget.color.withAlpha((_animation.value * 80).toInt()),
              blurRadius: widget.size * 1.5,
              spreadRadius: 1,
            ),
          ],
        ),
      ),
    );
  }
}
